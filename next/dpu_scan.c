/*
 * dpu_scan.c
 *
 * Sequential scan accelerated with DPU processors
 * ----
 * Copyright 2011-2022 (C) KaiGai Kohei <kaigai@kaigai.gr.jp>
 * Copyright 2014-2022 (C) PG-Strom Developers Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the PostgreSQL License.
 */
#include "pg_strom.h"

/* static variables */
static set_rel_pathlist_hook_type set_rel_pathlist_next = NULL;
static CustomPathMethods	dpuscan_path_methods;
static CustomScanMethods	dpuscan_plan_methods;
static CustomExecMethods	dpuscan_exec_methods;
static bool					enable_dpuscan;		/* GUC */

/*
 * DpuScanAddScanPath
 */
static void
DpuScanAddScanPath(PlannerInfo *root,
				   RelOptInfo *baserel,
				   Index rtindex,
				   RangeTblEntry *rte)
{
	List	   *input_rels_tlist;
	List	   *dev_quals = NIL;
	List	   *host_quals = NIL;
	ParamPathInfo *param_info;
	ListCell   *lc;

	/* call the secondary hook */
	if (set_rel_pathlist_next)
		set_rel_pathlist_next(root, baserel, rtindex, rte);
	/* nothing to do, if either PG-Strom or DpuScan is not enabled */
	if (!pgstrom_enabled || !enable_dpuscan)
		return;
	/* We already proved the relation empty, so nothing more to do */
	if (is_dummy_rel(baserel))
		return;
	/* It is the role of built-in Append node */
	if (rte->inh)
		return;

	/*
	 * check whether the qualifier can run on DPU device
	 */
	input_rels_tlist = list_make1(makeInteger(baserel->relid));
	foreach (lc, baserel->baserestrictinfo)
	{
		RestrictInfo *rinfo = lfirst(lc);

		if (pgstrom_dpu_expression(rinfo->clause,
								   input_rels_tlist,
								   NULL))
			dev_quals = lappend(dev_quals, rinfo);
		else
			host_quals = lappend(host_quals, rinfo);
	}
	/*
	 * check parametalized qualifiers
	 */
	param_info = get_baserel_parampathinfo(root, baserel,
										   baserel->lateral_relids);
	if (param_info)
	{
		foreach (lc, param_info->ppi_clauses)
		{
			RestrictInfo *rinfo = lfirst(lc);

			if (pgstrom_gpu_expression(rinfo->clause,
									   input_rels_tlist,
									   NULL))
				dev_quals = lappend(dev_quals, rinfo);
			else
				host_quals = lappend(host_quals, rinfo);
		}
	}

	 /* Creation of DpuScan path */
	for (int try_parallel=0; try_parallel < 2; try_parallel++)
	{
		pgstromPlanInfo pp_data;
		pgstromPlanInfo *pp_info;
		CustomPath	   *cpath;
		ParamPathInfo  *param_info = NULL;
		int				parallel_nworkers = 0;
		Cost			startup_cost = 0.0;
		Cost			run_cost = 0.0;
		Cost			final_cost = 0.0;

		memset(&pp_data, 0, sizeof(pgstromPlanInfo));
		if (!considerXpuScanPathParams(root,
									   baserel,
									   DEVKIND__NVIDIA_DPU,
									   try_parallel > 0,	/* parallel_aware */
									   dev_quals,
									   host_quals,
									   &parallel_nworkers,
									   &pp_data.brin_index_oid,
									   &pp_data.brin_index_conds,
									   &pp_data.brin_index_quals,
									   &startup_cost,
									   &run_cost,
									   &final_cost,
									   NULL,
									   NULL,
									   &pp_data.ds_entry))
			return;

		/* setup DpuScanInfo (Path phase) */
		pp_info = pmemdup(&pp_data, sizeof(pgstromPlanInfo));
		cpath = makeNode(CustomPath);
		cpath->path.pathtype = T_CustomScan;
		cpath->path.parent = baserel;
		cpath->path.pathtarget = baserel->reltarget;
		cpath->path.param_info = param_info;
		cpath->path.parallel_aware = (try_parallel > 0);
		cpath->path.parallel_safe = baserel->consider_parallel;
		cpath->path.parallel_workers = parallel_nworkers;
		cpath->path.rows = (param_info ? param_info->ppi_rows : baserel->rows);
		cpath->path.startup_cost = startup_cost;
		cpath->path.total_cost = startup_cost + run_cost + final_cost;
		cpath->path.pathkeys = NIL; /* unsorted results */
		cpath->flags = CUSTOMPATH_SUPPORT_PROJECTION;
		cpath->custom_paths = NIL;
		cpath->custom_private = list_make1(pp_info);
		cpath->methods = &dpuscan_path_methods;

		if (custom_path_remember(root, baserel, (try_parallel > 0), cpath))
		{
			if (try_parallel == 0)
				add_path(baserel, &cpath->path);
			else
				add_partial_path(baserel, &cpath->path);
		}
	}
}

/*
 * PlanDpuScanPath
 */
static Plan *
PlanDpuScanPath(PlannerInfo *root,
                RelOptInfo *baserel,
                CustomPath *best_path,
                List *tlist,
                List *clauses,
                List *custom_children)
{
	pgstromPlanInfo *pp_info = linitial(best_path->custom_private);
	CustomScan *cscan;

	/* sanity checks */
	Assert(baserel->relid > 0 &&
		   baserel->rtekind == RTE_RELATION &&
		   custom_children == NIL);
	cscan = PlanXpuScanPathCommon(root,
								  baserel,
								  best_path,
								  tlist,
								  clauses,
								  pp_info,
								  &dpuscan_plan_methods);
	form_pgstrom_plan_info(cscan, pp_info);

	return &cscan->scan.plan;
}

/*
 * CreateDpuScanState
 */
static Node *
CreateDpuScanState(CustomScan *cscan)
{
	pgstromTaskState *pts = palloc0(sizeof(pgstromTaskState));

	Assert(cscan->methods == &dpuscan_plan_methods);
	NodeSetTag(pts, T_CustomScanState);
	pts->css.flags = cscan->flags;
	pts->css.methods = &dpuscan_exec_methods;
	pts->devkind = DEVKIND__NVIDIA_DPU;
	pts->pp_info = deform_pgstrom_plan_info(cscan);

	return (Node *)pts;
}

/*
 * ExecInitDpuScan
 */
static void
ExecInitDpuScan(CustomScanState *node, EState *estate, int eflags)
{
	pgstromTaskState *pts = (pgstromTaskState *)node;
	pgstromPlanInfo	 *pp_info = pts->pp_info;

	 /* sanity checks */
    Assert(relation != NULL &&
           outerPlanState(node) == NULL &&
           innerPlanState(node) == NULL);
	pgstromExecInitTaskState(pts,
							 DEVKIND__NVIDIA_DPU,
							 pp_info->scan_quals,
							 pp_info->outer_refs,
							 pp_info->brin_index_oid,
                             pp_info->brin_index_conds,
                             pp_info->brin_index_quals);
	pts->cb_cpu_fallback = ExecFallbackCpuScan;
}

/*
 * DpuScanReCheckTuple
 */
static bool
DpuScanReCheckTuple(pgstromTaskState *pts, TupleTableSlot *epq_slot)
{
	/*
	 * NOTE: Only immutable operators/functions are executable
	 * on DPU devices, so its decision will never changed.
	 */
	return true;
}

/*
 * ExecDpuScan
 */
static TupleTableSlot *
ExecDpuScan(CustomScanState *node)
{
	pgstromTaskState *pts = (pgstromTaskState *)node;
	pgstromPlanInfo *pp_info = pts->pp_info;

	if (!pts->ps_state)
		pgstromSharedStateInitDSM(pts, NULL, NULL);
	if (!pts->conn)
	{
		const XpuCommand *session;

		session = pgstromBuildSessionInfo(pts,
										  pp_info->used_params,
										  pp_info->extra_bufsz,
										  pp_info->kvars_depth,
										  pp_info->kvars_resno,
										  pp_info->kexp_scan_kvars_load,
										  pp_info->kexp_scan_quals,
										  NULL,	/* join-load-vars */
										  NULL,	/* join-quals */
										  NULL,	/* hash-values */
										  NULL,	/* gist-join */
										  pp_info->kexp_projection,
										  0);	/* No join_inner_handle */
		DpuClientOpenSession(pts, session);
	}
	return ExecScan(&node->ss,
					(ExecScanAccessMtd) pgstromExecTaskState,
					(ExecScanRecheckMtd) DpuScanReCheckTuple);
}

/*
 * ExecEndDpuScan
 */
static void
ExecEndDpuScan(CustomScanState *node)
{
	pgstromTaskState *pts = (pgstromTaskState *)node;

	pgstromExecEndTaskState(pts);
}

/*
 * ExecReScanDpuScan
 */
static void
ExecReScanDpuScan(CustomScanState *node)
{
	pgstromTaskState *pts = (pgstromTaskState *)node;

	pgstromExecResetTaskState(pts);
}

/*
 * EstimateDpuScanDSM
 */
static Size
EstimateDpuScanDSM(CustomScanState *node,
				   ParallelContext *pcxt)
{
	pgstromTaskState *pts = (pgstromTaskState *)node;

	return pgstromSharedStateEstimateDSM(pts);
}

/*
 * InitializeDpuScanDSM
 */
static void
InitializeDpuScanDSM(CustomScanState *node,
					 ParallelContext *pcxt,
					 void *dsm_addr)
{
	pgstromTaskState *pts = (pgstromTaskState *)node;

	pgstromSharedStateInitDSM(pts, pcxt, dsm_addr);
}

/*
 * InitDpuScanWorker
 */
static void
InitDpuScanWorker(CustomScanState *node, shm_toc *toc, void *dsm_addr)
{
	pgstromTaskState *pts = (pgstromTaskState *)node;

	pgstromSharedStateAttachDSM(pts, dsm_addr);
}

/*
 * ExecShutdownDpuScan
 */
static void
ExecShutdownDpuScan(CustomScanState *node)
{
	pgstromTaskState *pts = (pgstromTaskState *)node;

	pgstromSharedStateShutdownDSM(pts);
}

/*
 * ExplainDpuScan
 */
static void
ExplainDpuScan(CustomScanState *node,
               List *ancestors,
               ExplainState *es)
{
	pgstromTaskState *pts = (pgstromTaskState *) node;
	List	   *dcontext;

	dcontext = set_deparse_context_plan(es->deparse_cxt,
										node->ss.ps.plan,
										ancestors);
	pgstromTaskStateExplain(pts, es, dcontext, "DPU");
}

/*
 * pgstrom_init_dpu_scan
 */
void
pgstrom_init_dpu_scan(void)
{
	/* pg_strom.enable_dpuscan */
	DefineCustomBoolVariable("pg_strom.enable_dpuscan",
							 "Enables the use of DPU accelerated full-scan",
							 NULL,
							 &enable_dpuscan,
							 true,
							 PGC_USERSET,
							 GUC_NOT_IN_SAMPLE,
							 NULL, NULL, NULL);
	/* setup path methods */
	memset(&dpuscan_path_methods, 0, sizeof(dpuscan_path_methods));
	dpuscan_path_methods.CustomName			= "DpuScan";
	dpuscan_path_methods.PlanCustomPath		= PlanDpuScanPath;

	/* setup plan methods */
	memset(&dpuscan_plan_methods, 0, sizeof(dpuscan_plan_methods));
	dpuscan_plan_methods.CustomName			= "DpuScan";
	dpuscan_plan_methods.CreateCustomScanState = CreateDpuScanState;
	RegisterCustomScanMethods(&dpuscan_plan_methods);

	/* setup exec methods */
	memset(&dpuscan_exec_methods, 0, sizeof(dpuscan_exec_methods));
    dpuscan_exec_methods.CustomName			= "DpuScan";
    dpuscan_exec_methods.BeginCustomScan	= ExecInitDpuScan;
    dpuscan_exec_methods.ExecCustomScan		= ExecDpuScan;
    dpuscan_exec_methods.EndCustomScan		= ExecEndDpuScan;
    dpuscan_exec_methods.ReScanCustomScan	= ExecReScanDpuScan;
    dpuscan_exec_methods.EstimateDSMCustomScan = EstimateDpuScanDSM;
    dpuscan_exec_methods.InitializeDSMCustomScan = InitializeDpuScanDSM;
    dpuscan_exec_methods.InitializeWorkerCustomScan = InitDpuScanWorker;
    dpuscan_exec_methods.ShutdownCustomScan = ExecShutdownDpuScan;
    dpuscan_exec_methods.ExplainCustomScan	= ExplainDpuScan;

    /* hook registration */
    set_rel_pathlist_next = set_rel_pathlist_hook;
    set_rel_pathlist_hook = DpuScanAddScanPath;
}
