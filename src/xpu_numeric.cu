/*
 * xpu_numeric.cu
 *
 * collection of numeric type support for both of GPU and DPU
 * ----
 * Copyright 2011-2023 (C) KaiGai Kohei <kaigai@kaigai.gr.jp>
 * Copyright 2014-2023 (C) PG-Strom Developers Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the PostgreSQL License.
 */
#include "xpu_common.h"
#include <math.h>

INLINE_FUNCTION(int)
xpu_numeric_sign(xpu_numeric_t *num)
{
	if (num->kind != XPU_NUMERIC_KIND__VALID)
	{
		Assert(num->kind != XPU_NUMERIC_KIND__NAN);
		/* Must be +Inf or -Inf */
		return num->kind == XPU_NUMERIC_KIND__POS_INF ? INT_MAX : INT_MIN;
	}
	if (num->value > 0)
		return 1;
	if (num->value < 0)
		return -1;
	return 0;
}

STATIC_FUNCTION(bool)
xpu_numeric_datum_ref(kern_context *kcxt,
					  xpu_datum_t *__result,
					  int vclass,
					  const kern_variable *kvar)
{
	xpu_numeric_t  *result = (xpu_numeric_t *)__result;
	const char	   *errmsg;

	if (vclass == KVAR_CLASS__VARLENA)
	{
		errmsg = __xpu_numeric_from_varlena(result, (const varlena *)kvar->ptr);
		if (!errmsg)
			return true;
		result->expr_ops = NULL;
		STROM_ELOG(kcxt, errmsg);
	}
	else
	{
		STROM_ELOG(kcxt, "unexpected vclass for device numeric data type.");
	}
	return false;
}

PUBLIC_FUNCTION(bool)
xpu_numeric_datum_store(kern_context *kcxt,
						const xpu_datum_t *xdatum,
						int *p_vclass,
						kern_variable *p_kvar)
{
	xpu_numeric_t  *num;

	num = (xpu_numeric_t *)kcxt_alloc(kcxt, sizeof(xpu_numeric_t));
	if (!num)
		return false;
	memcpy(num, xdatum, sizeof(xpu_geometry_t));
	assert(num->expr_ops == &xpu_numeric_ops);

	*p_vclass = KVAR_CLASS__XPU_DATUM;
	p_kvar->ptr = (void *)num;
	return true;
}

STATIC_FUNCTION(int)
xpu_numeric_datum_write(kern_context *kcxt,
						char *buffer,
						const xpu_datum_t *__arg)
{
	const xpu_numeric_t *arg = (const xpu_numeric_t *)__arg;

	if (XPU_DATUM_ISNULL(arg))
		return 0;
	if (arg->kind != XPU_NUMERIC_KIND__VALID)
	{
		int		sz = offsetof(NumericData, choice.n_header) + sizeof(uint16_t);

		if (buffer)
		{
			NumericChoice *nc = (NumericChoice *)buffer;

			if (arg->kind == XPU_NUMERIC_KIND__POS_INF)
				nc->n_header = NUMERIC_PINF;
			else if (arg->kind == XPU_NUMERIC_KIND__NEG_INF)
				nc->n_header = NUMERIC_NINF;
			else
				nc->n_header = NUMERIC_NAN;
			SET_VARSIZE(nc, sz);
		}
		return sz;
	}
	return __xpu_numeric_to_varlena(buffer, arg->weight, arg->value);
}

PUBLIC_FUNCTION(bool)
xpu_numeric_datum_hash(kern_context *kcxt,
					   uint32_t *p_hash,
					   const xpu_datum_t *__arg)
{
	const xpu_numeric_t *arg = (const xpu_numeric_t *)__arg;

	if (XPU_DATUM_ISNULL(arg))
		*p_hash = 0;
	else if (arg->kind != XPU_NUMERIC_KIND__VALID)
		*p_hash = pg_hash_any(&arg->kind, sizeof(uint8_t));
	else
		*p_hash = (pg_hash_any(&arg->weight, sizeof(int16_t)) ^
				   pg_hash_any(&arg->value, sizeof(int128_t)));
	return true;
}

STATIC_FUNCTION(int)
__numeric_compare(const xpu_numeric_t *a, const xpu_numeric_t *b);

STATIC_FUNCTION(bool)
xpu_numeric_datum_comp(kern_context *kcxt,
					   int *p_comp,
					   const xpu_datum_t *__a,
					   const xpu_datum_t *__b)
{
	const xpu_numeric_t *a = (const xpu_numeric_t *)__a;
	const xpu_numeric_t *b = (const xpu_numeric_t *)__b;

	assert(!XPU_DATUM_ISNULL(a) && !XPU_DATUM_ISNULL(b));
	*p_comp = __numeric_compare(a, b);
	return true;
}
PGSTROM_SQLTYPE_OPERATORS(numeric, false, 4, -1);

PUBLIC_FUNCTION(bool)
__xpu_numeric_to_int64(kern_context *kcxt,
					   int64_t *p_ival,
					   const xpu_numeric_t *num,
					   int64_t min_value,
					   int64_t max_value)
{
	int128_t	ival;
	int16_t		weight;

	assert(num->expr_ops == &xpu_numeric_ops);

	if (num->kind != XPU_NUMERIC_KIND__VALID)
	{
		STROM_ELOG(kcxt, "cannot convert NaN/Inf to integer");
		return false;
	}

	ival = num->value;
	weight = num->weight;
	if (ival != 0)
	{
		while (weight > 0)
		{
			/* round of 0.x digit */
			if (weight == 1)
				ival += (ival > 0 ? 5 : -5);
			ival /= 10;
			weight--;
		}
		while (weight < 0)
		{
			ival *= 10;
			weight++;
			if (ival < min_value || ival > max_value)
				break;
		}
		if (ival < min_value || ival > max_value)
		{
			STROM_ELOG(kcxt, "integer out of range");
			return false;
		}
	}
	*p_ival = ival;
	return true;
}

#define PG_NUMERIC_TO_INT_TEMPLATE(TARGET,MIN_VALUE,MAX_VALUE)		\
	PUBLIC_FUNCTION(bool)											\
	pgfn_numeric_to_##TARGET(XPU_PGFUNCTION_ARGS)					\
	{																\
		xpu_##TARGET##_t *result = (xpu_##TARGET##_t *)__result;	\
		xpu_numeric_t	num;										\
		int64_t			ival;										\
		const kern_expression *karg = KEXP_FIRST_ARG(kexp);			\
																	\
		assert(kexp->nr_args == 1 &&								\
			   KEXP_IS_VALID(karg, numeric));						\
		if (!EXEC_KERN_EXPRESSION(kcxt, karg, &num))				\
			return false;											\
		if (XPU_DATUM_ISNULL(&num))									\
		{															\
			result->expr_ops = NULL;								\
		}															\
		if (!__xpu_numeric_to_int64(kcxt, &ival, &num,				\
									MIN_VALUE, MAX_VALUE))			\
			return false;											\
		result->value = ival;										\
		result->expr_ops = &xpu_##TARGET##_ops;						\
		return true;												\
	}
PG_NUMERIC_TO_INT_TEMPLATE(int1,SCHAR_MIN,SCHAR_MAX)
PG_NUMERIC_TO_INT_TEMPLATE(int2,SHRT_MIN,SHRT_MAX)
PG_NUMERIC_TO_INT_TEMPLATE(int4,INT_MIN,INT_MAX)
PG_NUMERIC_TO_INT_TEMPLATE(int8,LLONG_MIN,LLONG_MAX)
PG_NUMERIC_TO_INT_TEMPLATE(money,LLONG_MIN,LLONG_MAX)

PUBLIC_FUNCTION(bool)
__xpu_numeric_to_fp64(kern_context *kcxt,
					  float8_t *p_fval,
					  const xpu_numeric_t *num)
{
	if (num->kind == XPU_NUMERIC_KIND__VALID)
	{
		float8_t	fval = num->value;
		int16_t		weight = num->weight;

		if (fval != 0.0)
		{
			while (weight > 0)
			{
				fval /= 10.0;
				weight--;
			}
			while (weight < 0)
			{
				fval *= 10.0;
				weight++;
			}
			if (isnan(fval) || isinf(fval))
			{
				STROM_ELOG(kcxt,"float out of range");
				return false;
			}
		}
		*p_fval = fval;
	}
	else if (num->kind == XPU_NUMERIC_KIND__POS_INF)
		*p_fval = INFINITY;
	else if (num->kind == XPU_NUMERIC_KIND__NEG_INF)
		*p_fval = -INFINITY;
	else
		*p_fval = NAN;

	return true;
}

#define PG_NUMERIC_TO_FLOAT_TEMPLATE(TARGET,__CAST)					\
	PUBLIC_FUNCTION(bool)											\
	pgfn_numeric_to_##TARGET(XPU_PGFUNCTION_ARGS)					\
	{																\
		xpu_##TARGET##_t *result = (xpu_##TARGET##_t *)__result;	\
		xpu_numeric_t	num;										\
		const kern_expression *karg = KEXP_FIRST_ARG(kexp);			\
																	\
		assert(kexp->nr_args == 1 &&								\
			   KEXP_IS_VALID(karg, numeric));						\
		if (!EXEC_KERN_EXPRESSION(kcxt, karg, &num))				\
			return false;											\
		if (XPU_DATUM_ISNULL(&num))									\
			result->expr_ops = NULL;								\
		else														\
		{															\
			float8_t	fval;										\
																	\
			if (!__xpu_numeric_to_fp64(kcxt, &fval, &num))			\
				return false;										\
			result->expr_ops = &xpu_##TARGET##_ops;					\
			result->value = __CAST(fval);							\
		}															\
		return true;												\
	}
PG_NUMERIC_TO_FLOAT_TEMPLATE(float2, __to_fp16)
PG_NUMERIC_TO_FLOAT_TEMPLATE(float4, __to_fp32)
PG_NUMERIC_TO_FLOAT_TEMPLATE(float8, __to_fp64)

#define PG_INT_TO_NUMERIC_TEMPLATE(SOURCE)							\
	PUBLIC_FUNCTION(bool)											\
	pgfn_##SOURCE##_to_numeric(XPU_PGFUNCTION_ARGS)					\
	{																\
		xpu_numeric_t	   *result = (xpu_numeric_t *)__result;		\
		xpu_##SOURCE##_t	datum;									\
		const kern_expression *karg = KEXP_FIRST_ARG(kexp);			\
																	\
		assert(kexp->nr_args == 1 &&								\
			   KEXP_IS_VALID(karg,SOURCE));							\
		if (!EXEC_KERN_EXPRESSION(kcxt, karg, &datum))				\
			return false;											\
		if (XPU_DATUM_ISNULL(&datum))								\
			result->expr_ops = NULL;								\
		else														\
		{															\
			result->expr_ops = &xpu_numeric_ops;					\
			set_normalized_numeric(result, datum.value, 0);			\
		}															\
		return true;												\
	}
PG_INT_TO_NUMERIC_TEMPLATE(int1)
PG_INT_TO_NUMERIC_TEMPLATE(int2)
PG_INT_TO_NUMERIC_TEMPLATE(int4)
PG_INT_TO_NUMERIC_TEMPLATE(int8)
PG_INT_TO_NUMERIC_TEMPLATE(money)

#define PG_FLOAT_TO_NUMERIC_TEMPLATE(SOURCE,__TYPE,__CAST,			\
									 __MODF,__RINTL)				\
	PUBLIC_FUNCTION(bool)											\
	pgfn_##SOURCE##_to_numeric(XPU_PGFUNCTION_ARGS)					\
	{																\
		xpu_numeric_t	   *result = (xpu_numeric_t *)__result;		\
		xpu_##SOURCE##_t	datum;									\
		__TYPE				fval;									\
		const kern_expression *karg = KEXP_FIRST_ARG(kexp);			\
																	\
		assert(kexp->nr_args == 1 &&								\
			   KEXP_IS_VALID(karg,SOURCE));							\
		if (!EXEC_KERN_EXPRESSION(kcxt, karg, &datum))				\
			return false;											\
		if (XPU_DATUM_ISNULL(&datum))								\
			result->expr_ops = NULL;								\
		else														\
		{															\
			result->expr_ops = &xpu_numeric_ops;					\
			fval = __CAST(datum.value);								\
			if (isinf(fval))										\
				result->kind = (fval > 0.0							\
								? XPU_NUMERIC_KIND__POS_INF			\
								: XPU_NUMERIC_KIND__NEG_INF);		\
			else if (isnan(fval))									\
				result->kind = XPU_NUMERIC_KIND__NAN;				\
			else													\
			{														\
				__TYPE		a,b = __MODF(fval, &a);					\
				int128_t	value = (int128_t)a;					\
				int16_t		weight = 0;								\
				bool		negative = (value < 0);					\
																	\
				if (negative)										\
					value = -value;									\
				while (b != 0.0 && (value>>124) == 0)				\
				{													\
					b = __MODF(b * 10.0, &a);						\
					value = 10 * value + (int128_t)a;				\
					weight++;										\
				}													\
				set_normalized_numeric(result,value,weight);		\
			}														\
		}															\
		return true;												\
	}
PG_FLOAT_TO_NUMERIC_TEMPLATE(float2, float,__to_fp32,modff,rintf)
PG_FLOAT_TO_NUMERIC_TEMPLATE(float4, float,__to_fp32,modff,rintf)
PG_FLOAT_TO_NUMERIC_TEMPLATE(float8,double,__to_fp64,modf, rint)

STATIC_FUNCTION(int)
__numeric_compare(const xpu_numeric_t *a, const xpu_numeric_t *b)
{
	int128_t	a_val = a->value;
	int128_t	b_val = b->value;
	int16_t		a_weight = a->weight;
	int16_t		b_weight = b->weight;

	/* If any NaN or Inf */
	if (a->kind != XPU_NUMERIC_KIND__VALID)
	{
		if (a->kind == XPU_NUMERIC_KIND__NAN)
		{
			if (b->kind == XPU_NUMERIC_KIND__NAN)
				return 0;	/* NaN == Nan */
			return 1;		/* NaN > non-NaN */
		}
		else if (a->kind == XPU_NUMERIC_KIND__POS_INF)
		{
			if (b->kind == XPU_NUMERIC_KIND__NAN)
				return -1;	/* +Inf < NaN */
			if (b->kind == XPU_NUMERIC_KIND__POS_INF)
				return 0;	/* +Inf == +Inf */
			return 1;		/* +Inf > anything else */
		}
		else
		{
			if (b->kind == XPU_NUMERIC_KIND__NEG_INF)
				return 0;	/* -Inf == -Inf */
			return -1;		/* -Inf < anything else */
		}
	}
	else if (b->kind != XPU_NUMERIC_KIND__VALID)
	{
		if (b->kind == XPU_NUMERIC_KIND__NEG_INF)
			return 1;		/* normal > -Inf */
		else
			return -1;		/* normal < NaN or +Inf */
	}
	else if ((a_val > 0 && b_val <= 0) || (a_val == 0 && b_val < 0))
		return 1;
	else if ((b_val > 0 && a_val <= 0) || (b_val == 0 && a_val < 0))
		return -1;
	/* Ok, both side are same sign with valid values */
	while (a_weight > b_weight)
	{
		b_val *= 10;
		b_weight++;
	}
	while (a_weight < b_weight)
	{
		a_val *= 10;
		a_weight++;
	}
	if (a_val > b_val)
		return 1;
	if (a_val < b_val)
		return -1;
	return 0;
}

#define PG_NUMERIC_COMPARE_TEMPLATE(NAME,OPER)							\
	PUBLIC_FUNCTION(bool)												\
	pgfn_numeric_##NAME(XPU_PGFUNCTION_ARGS)							\
	{																	\
		xpu_bool_t	   *result = (xpu_bool_t *)__result;				\
		xpu_numeric_t	datum_a, datum_b;								\
		const kern_expression *karg = KEXP_FIRST_ARG(kexp);				\
																		\
		assert(kexp->nr_args == 2 &&									\
			   KEXP_IS_VALID(karg,numeric));							\
		if (!EXEC_KERN_EXPRESSION(kcxt, karg, &datum_a))				\
			return false;												\
		karg = KEXP_NEXT_ARG(karg);										\
		assert(KEXP_IS_VALID(karg, numeric));							\
		if (!EXEC_KERN_EXPRESSION(kcxt, karg, &datum_b))				\
			return false;												\
		if (XPU_DATUM_ISNULL(&datum_a) || XPU_DATUM_ISNULL(&datum_b))	\
		{																\
			__pg_simple_nullcomp_##NAME(&datum_a, &datum_b);			\
		}																\
		else															\
		{																\
			result->expr_ops = &xpu_numeric_ops;						\
			result->value = (__numeric_compare(&datum_a,				\
											   &datum_b) OPER 0);		\
		}																\
		return true;													\
	}
PG_NUMERIC_COMPARE_TEMPLATE(eq, ==)
PG_NUMERIC_COMPARE_TEMPLATE(ne, !=)
PG_NUMERIC_COMPARE_TEMPLATE(lt, <)
PG_NUMERIC_COMPARE_TEMPLATE(le, <=)
PG_NUMERIC_COMPARE_TEMPLATE(gt, >)
PG_NUMERIC_COMPARE_TEMPLATE(ge, >=)

PUBLIC_FUNCTION(bool)
pgfn_numeric_add(XPU_PGFUNCTION_ARGS)
{
	xpu_numeric_t  *result = (xpu_numeric_t *)__result;
	xpu_numeric_t	datum_a;
	xpu_numeric_t	datum_b;
	const kern_expression *karg = KEXP_FIRST_ARG(kexp);

	assert(kexp->nr_args == 2 &&
		   KEXP_IS_VALID(karg, numeric));
	if (!EXEC_KERN_EXPRESSION(kcxt, karg, &datum_a))
		return false;
	karg = KEXP_NEXT_ARG(karg);
	assert(KEXP_IS_VALID(karg, numeric));
	if (!EXEC_KERN_EXPRESSION(kcxt, karg, &datum_b))
		return false;
	if (XPU_DATUM_ISNULL(&datum_a) || XPU_DATUM_ISNULL(&datum_b))
		result->expr_ops = NULL;
	else
	{
		result->expr_ops = &xpu_numeric_ops;

		if (datum_a.kind != XPU_NUMERIC_KIND__VALID ||
			datum_b.kind != XPU_NUMERIC_KIND__VALID)
		{
			if (datum_a.kind == XPU_NUMERIC_KIND__NAN ||
				datum_b.kind == XPU_NUMERIC_KIND__NAN)
				result->kind = XPU_NUMERIC_KIND__NAN;
			else if (datum_a.kind == XPU_NUMERIC_KIND__POS_INF)
			{
				if (datum_b.kind == XPU_NUMERIC_KIND__NEG_INF)
					result->kind = XPU_NUMERIC_KIND__NAN;	/* Inf - Inf */
				else
					result->kind = XPU_NUMERIC_KIND__POS_INF;
			}
			else if (datum_a.kind == XPU_NUMERIC_KIND__NEG_INF)
			{
				if (datum_b.kind == XPU_NUMERIC_KIND__POS_INF)
					result->kind = XPU_NUMERIC_KIND__NAN;	/* -Inf + Inf */
				else
					result->kind = XPU_NUMERIC_KIND__NEG_INF;
			}
			else if (datum_b.kind == XPU_NUMERIC_KIND__POS_INF)
				result->kind = XPU_NUMERIC_KIND__POS_INF;
			else
				result->kind = XPU_NUMERIC_KIND__NEG_INF;
		}
		else
		{
			while (datum_a.weight > datum_b.weight)
			{
				datum_b.value *= 10;
				datum_b.weight++;
			}
			while (datum_a.weight < datum_b.weight)
			{
				datum_a.value *= 10;
				datum_a.weight++;
			}
			set_normalized_numeric(result,
								   datum_a.value + datum_b.value,
								   datum_a.weight);
		}
	}
	return true;
}

PUBLIC_FUNCTION(bool)
pgfn_numeric_sub(XPU_PGFUNCTION_ARGS)
{
	xpu_numeric_t  *result = (xpu_numeric_t *)__result;
	xpu_numeric_t	datum_a;
	xpu_numeric_t	datum_b;
	const kern_expression *karg = KEXP_FIRST_ARG(kexp);

	assert(kexp->nr_args == 2 &&
		   KEXP_IS_VALID(karg, numeric));
	if (!EXEC_KERN_EXPRESSION(kcxt, karg, &datum_a))
		return false;
	karg = KEXP_NEXT_ARG(karg);
	assert(KEXP_IS_VALID(karg, numeric));
	if (!EXEC_KERN_EXPRESSION(kcxt, karg, &datum_b))
		return false;
	if (XPU_DATUM_ISNULL(&datum_a) || XPU_DATUM_ISNULL(&datum_b))
		result->expr_ops = NULL;
	else
	{
		result->expr_ops = &xpu_numeric_ops;

		if (datum_a.kind != XPU_NUMERIC_KIND__VALID ||
			datum_b.kind != XPU_NUMERIC_KIND__VALID)
		{
			if (datum_a.kind == XPU_NUMERIC_KIND__NAN ||
				datum_b.kind == XPU_NUMERIC_KIND__NAN)
				result->kind = XPU_NUMERIC_KIND__NAN;
			else if (datum_a.kind == XPU_NUMERIC_KIND__POS_INF)
			{
				if (datum_b.kind == XPU_NUMERIC_KIND__POS_INF)
					result->kind = XPU_NUMERIC_KIND__NAN;	/* Inf - Inf */
				else
					result->kind = XPU_NUMERIC_KIND__POS_INF;
			}
			else if (datum_a.kind == XPU_NUMERIC_KIND__NEG_INF)
			{
				if (datum_b.kind == XPU_NUMERIC_KIND__NEG_INF)
					result->kind = XPU_NUMERIC_KIND__NAN;	/* -Inf - -Inf*/
				else
					result->kind = XPU_NUMERIC_KIND__NEG_INF;
			}
			else if (datum_b.kind == XPU_NUMERIC_KIND__POS_INF)
				result->kind = XPU_NUMERIC_KIND__NEG_INF;
			else
				result->kind = XPU_NUMERIC_KIND__POS_INF;
		}
		else
		{
			while (datum_a.weight > datum_b.weight)
			{
				datum_b.value *= 10;
				datum_b.weight++;
			}
			while (datum_a.weight < datum_b.weight)
			{
				datum_a.value *= 10;
				datum_a.weight++;
			}
			set_normalized_numeric(result,
								   datum_a.value - datum_b.value,
								   datum_a.weight);
		}
	}
	return true;
}

PUBLIC_FUNCTION(bool)
pgfn_numeric_mul(XPU_PGFUNCTION_ARGS)
{
	xpu_numeric_t  *result = (xpu_numeric_t *)__result;
	xpu_numeric_t	datum_a;
	xpu_numeric_t	datum_b;
	const kern_expression *karg = KEXP_FIRST_ARG(kexp);

	assert(kexp->nr_args == 2 &&
		   KEXP_IS_VALID(karg, numeric));
	if (!EXEC_KERN_EXPRESSION(kcxt, karg, &datum_a))
		return false;
	karg = KEXP_NEXT_ARG(karg);
	assert(KEXP_IS_VALID(karg, numeric));
	if (!EXEC_KERN_EXPRESSION(kcxt, karg, &datum_b))
		return false;
	if (XPU_DATUM_ISNULL(&datum_a) || XPU_DATUM_ISNULL(&datum_b))
		result->expr_ops = NULL;
	else
	{
		result->expr_ops = &xpu_numeric_ops;

		if (datum_a.kind != XPU_NUMERIC_KIND__VALID ||
			datum_b.kind != XPU_NUMERIC_KIND__VALID)
		{
			if (datum_a.kind == XPU_NUMERIC_KIND__NAN ||
				datum_a.kind == XPU_NUMERIC_KIND__NAN)
			{
				result->kind = XPU_NUMERIC_KIND__NAN;
			}
			else if (datum_a.kind == XPU_NUMERIC_KIND__POS_INF)
			{
				int		__sign = xpu_numeric_sign(&datum_b);

				if (__sign < 0)
					result->kind = XPU_NUMERIC_KIND__NEG_INF;
				else if (__sign > 0)
					result->kind = XPU_NUMERIC_KIND__POS_INF;
				else
					result->kind = XPU_NUMERIC_KIND__NAN;
			}
			else if (datum_a.kind == XPU_NUMERIC_KIND__NEG_INF)
			{
				int		__sign = xpu_numeric_sign(&datum_b);

				if (__sign < 0)
					result->kind = XPU_NUMERIC_KIND__POS_INF;
				else if (__sign > 0)
					result->kind = XPU_NUMERIC_KIND__NEG_INF;
				else
					result->kind = XPU_NUMERIC_KIND__NAN;
			}
			else if (datum_b.kind == XPU_NUMERIC_KIND__POS_INF)
			{
				int		__sign = xpu_numeric_sign(&datum_a);

				if (__sign < 0)
					result->kind = XPU_NUMERIC_KIND__NEG_INF;
				else if (__sign > 0)
					result->kind = XPU_NUMERIC_KIND__POS_INF;
				else
					result->kind = XPU_NUMERIC_KIND__NAN;
			}
			else
			{
				int		__sign = xpu_numeric_sign(&datum_a);

				if (__sign < 0)
					result->kind = XPU_NUMERIC_KIND__POS_INF;
				else if (__sign > 0)
					result->kind = XPU_NUMERIC_KIND__NEG_INF;
				else
					result->kind = XPU_NUMERIC_KIND__NAN;
			}
		}
		else
		{
			set_normalized_numeric(result,
								   datum_a.value * datum_b.value,
								   datum_a.weight + datum_b.weight);
		}
	}
	return true;
}

PUBLIC_FUNCTION(bool)
pgfn_numeric_div(XPU_PGFUNCTION_ARGS)
{
	xpu_numeric_t  *result = (xpu_numeric_t *)__result;
	xpu_numeric_t	datum_a;
	xpu_numeric_t	datum_b;
	const kern_expression *karg = KEXP_FIRST_ARG(kexp);

	assert(kexp->nr_args == 2 &&
		   KEXP_IS_VALID(karg, numeric));
	if (!EXEC_KERN_EXPRESSION(kcxt, karg, &datum_a))
		return false;
	karg = KEXP_NEXT_ARG(karg);
	assert(KEXP_IS_VALID(karg, numeric));
	if (!EXEC_KERN_EXPRESSION(kcxt, karg, &datum_b))
		return false;
	if (XPU_DATUM_ISNULL(&datum_a) || XPU_DATUM_ISNULL(&datum_b))
		result->expr_ops = NULL;
	else
	{
		result->expr_ops = &xpu_numeric_ops;

		if (datum_a.kind != XPU_NUMERIC_KIND__VALID ||
			datum_b.kind != XPU_NUMERIC_KIND__VALID)
		{
			if (datum_a.kind == XPU_NUMERIC_KIND__NAN ||
				datum_b.kind == XPU_NUMERIC_KIND__NAN)
			{
				result->kind = XPU_NUMERIC_KIND__NAN;
			}
			else if (datum_a.kind == XPU_NUMERIC_KIND__POS_INF)
			{
				int		__sign = xpu_numeric_sign(&datum_b);
				if (__sign == 1)
					result->kind = XPU_NUMERIC_KIND__NEG_INF;
				else if (__sign == -1)
					result->kind = XPU_NUMERIC_KIND__POS_INF;
				else if (__sign != 0)
					result->kind = XPU_NUMERIC_KIND__NAN;
				else
				{
					STROM_ELOG(kcxt, "division by zero");
					return false;
				}
			}
			else if (datum_a.kind == XPU_NUMERIC_KIND__NEG_INF)
			{
				int		__sign = xpu_numeric_sign(&datum_b);

				if (__sign == 1)
					result->kind = XPU_NUMERIC_KIND__NEG_INF;
				else if (__sign == -1)
					result->kind = XPU_NUMERIC_KIND__POS_INF;
				else if (__sign != 0)
					result->kind = XPU_NUMERIC_KIND__NAN;
				else
				{
					STROM_ELOG(kcxt, "division by zero");
					return false;
				}
			}
			else
			{
				/* by here, datum_a must be finite, so datum_b is not */
				set_normalized_numeric(result, 0, 0);
			}
		}
		else if (datum_b.value == 0)
		{
			STROM_ELOG(kcxt, "division by zero");
			return false;
		}
		else
		{
			int128_t	rem = datum_a.value;
			int128_t	div = datum_b.value;
			int128_t	x, ival = 0;
			int16_t		weight = datum_a.weight - datum_b.weight;
			bool		negative = false;

			if (rem < 0)
			{
				rem = -rem;
				if (div < 0)
					div = -div;
				else
					negative = true;
			}
			else if (div < 0)
			{
				negative = true;
				div = -div;
			}
			assert(rem >= 0 && div >= 0);

			for (;;)
			{
				x = rem / div;
				ival = 10 * ival + x;
				rem -= x * div;
				if (rem == 0)
					break;
				rem *= 10;
				weight++;
			}
			if (negative)
				ival = -ival;
			set_normalized_numeric(result, ival, weight);
		}
	}
	return true;
}

PUBLIC_FUNCTION(bool)
pgfn_numeric_mod(XPU_PGFUNCTION_ARGS)
{
	xpu_numeric_t  *result = (xpu_numeric_t *)__result;
	xpu_numeric_t	datum_a;
	xpu_numeric_t	datum_b;
	const kern_expression *karg = KEXP_FIRST_ARG(kexp);

	assert(kexp->nr_args == 2 &&
		   KEXP_IS_VALID(karg, numeric));
	if (!EXEC_KERN_EXPRESSION(kcxt, karg, &datum_a))
		return false;
	karg = KEXP_NEXT_ARG(karg);
	assert(KEXP_IS_VALID(karg, numeric));
	if (!EXEC_KERN_EXPRESSION(kcxt, karg, &datum_b))
		return false;
	if (XPU_DATUM_ISNULL(&datum_a) || XPU_DATUM_ISNULL(&datum_b))
		result->expr_ops = NULL;
	else
	{
		result->expr_ops = &xpu_numeric_ops;

		if (datum_a.kind != XPU_NUMERIC_KIND__VALID ||
			datum_b.kind != XPU_NUMERIC_KIND__VALID)
		{
			if (datum_a.kind == XPU_NUMERIC_KIND__NAN ||
				datum_b.kind == XPU_NUMERIC_KIND__NAN)
			{
				result->kind = XPU_NUMERIC_KIND__NAN;
			}
			else if (datum_a.kind == XPU_NUMERIC_KIND__POS_INF ||
					 datum_a.kind == XPU_NUMERIC_KIND__NEG_INF)
			{
				if (datum_b.kind == XPU_NUMERIC_KIND__VALID &&
					datum_b.value == 0)
				{
					STROM_ELOG(kcxt, "division by zero");
					return false;
				}
				else
				{
					result->kind = XPU_NUMERIC_KIND__NAN;
				}
			}
			else
			{
				/* num2 must be [-]Inf; result is num1 regardless of sign of num2 */
				result->kind = datum_b.kind;
				result->value = datum_b.value;
			}
		}
		else if (datum_b.value == 0)
		{
			STROM_ELOG(kcxt, "division by zero");
			return false;
		}
		else
		{
			while (datum_a.weight > datum_b.value)
			{
				datum_b.value *= 10;
				datum_b.weight++;
			}
			while (datum_a.weight < datum_b.weight)
			{
				datum_a.value *= 10;
				datum_a.weight++;
			}
			set_normalized_numeric(result,
								   datum_a.value % datum_b.value,
								   datum_a.weight);
		}
	}
	return true;
}

PUBLIC_FUNCTION(bool)
pgfn_numeric_uplus(XPU_PGFUNCTION_ARGS)
{
	const kern_expression *karg = KEXP_FIRST_ARG(kexp);

	assert(kexp->nr_args == 1 &&
		   KEXP_IS_VALID(karg, numeric));
	return EXEC_KERN_EXPRESSION(kcxt, karg, __result);
}

PUBLIC_FUNCTION(bool)
pgfn_numeric_uminus(XPU_PGFUNCTION_ARGS)
{
	xpu_numeric_t  *result = (xpu_numeric_t *)__result;
	const kern_expression *karg = KEXP_FIRST_ARG(kexp);

	assert(kexp->nr_args == 1 &&
		   KEXP_IS_VALID(karg, numeric));

	if (!EXEC_KERN_EXPRESSION(kcxt, karg, result))
		return false;
	if (!XPU_DATUM_ISNULL(result))
	{
		if (result->kind == XPU_NUMERIC_KIND__VALID)
			result->value = -result->value;
	}
	return true;
}

PUBLIC_FUNCTION(bool)
pgfn_numeric_abs(XPU_PGFUNCTION_ARGS)
{
	xpu_numeric_t  *result = (xpu_numeric_t *)__result;
	const kern_expression *karg = KEXP_FIRST_ARG(kexp);

	assert(kexp->nr_args == 1 &&
		   KEXP_IS_VALID(karg, numeric));
	if (!EXEC_KERN_EXPRESSION(kcxt, karg, result))
		return false;
	if (!XPU_DATUM_ISNULL(result))
	{
		if (result->kind == XPU_NUMERIC_KIND__VALID &&
			result->value < 0)
			result->value = -result->value;
	}
	return true;
}

PUBLIC_FUNCTION(int)
pg_numeric_to_cstring(kern_context *kcxt,
					  varlena *numeric,
					  char *buf, char *endp)
{
	NumericChoice *nc = (NumericChoice *)VARDATA_ANY(numeric);
	uint32_t	nc_len = VARSIZE_ANY_EXHDR(numeric);
	uint16_t	n_head = __Fetch(&nc->n_header);
	int			ndigits = NUMERIC_NDIGITS(n_head, nc_len);
	int			weight = NUMERIC_WEIGHT(nc, n_head);
	int			sign = NUMERIC_SIGN(n_head);
	int			dscale = NUMERIC_DSCALE(nc, n_head);
	int			d;
	char	   *cp = buf;
	NumericDigit *n_data = NUMERIC_DIGITS(nc, n_head);
	NumericDigit  dig, d1 __attribute__ ((unused));

	if (sign == NUMERIC_NEG)
	{
		if (cp >= endp)
			return -1;
		*cp++ = '-';
	}
	/* Output all digits before the decimal point */
	if (weight < 0)
	{
		d = weight + 1;
		if (cp >= endp)
			return -1;
		*cp++ = '0';
	}
	else
	{
		for (d = 0; d <= weight; d++)
		{
			bool		putit __attribute__ ((unused)) = (d > 0);

			if (d < ndigits)
				dig = __Fetch(n_data + d);
			else
				dig = 0;
#if PG_DEC_DIGITS == 4
			d1 = dig / 1000;
			dig -= d1 * 1000;
			putit |= (d1 > 0);
			if (putit)
			{
				if (cp >= endp)
					return -1;
				*cp++ = d1 + '0';
			}
			d1 = dig / 100;
			dig -= d1 * 100;
			putit |= (d1 > 0);
			if (putit)
			{
				if (cp >= endp)
					return -1;
				*cp++ = d1 + '0';
			}
			d1 = dig / 10;
			dig -= d1 * 10;
			putit |= (d1 > 0);
			if (putit)
			{
				if (cp >= endp)
					return -1;
				*cp++ = d1 + '0';
			}
			*cp++ = dig + '0';
#elif PG_DEC_DIGITS == 2
			d1 = dig / 10;
			dig -= d1 * 10;
			if (d1 > 0 || d > 0)
			{
				if (cp >= endp)
					return -1;
				*cp++ = d1 + '0';
			}
			if (cp >= endp)
				return -1;
			*cp++ = dig + '0';
#elif PG_DEC_DIGITS == 1
			if (cp >= endp)
				return -1;
			*cp++ = dig + '0';
#else
#error unsupported NBASE
#endif
		}
	}

	if (dscale > 0)
	{
		char   *lastp = cp;

		if (cp >= endp)
			return -1;
		*cp++ = '.';
		lastp = cp + dscale;
		for (int i = 0; i < dscale; d++, i += PG_DEC_DIGITS)
		{
			if (d >= 0 && d < ndigits)
				dig = __Fetch(n_data + d);
			else
				dig = 0;
#if PG_DEC_DIGITS == 4
			if (cp + 4 > endp)
				return -1;
			d1 = dig / 1000;
			dig -= d1 * 1000;
			*cp++ = d1 + '0';
			d1 = dig / 100;
			dig -= d1 * 100;
			*cp++ = d1 + '0';
			d1 = dig / 10;
			dig -= d1 * 10;
			*cp++ = d1 + '0';
			*cp++ = dig + '0';
			if (dig != 0)
				lastp = cp;
#elif PG_DEC_DIGITS == 2
			if (cp + 2 > endp)
				return -1;
			d1 = dig / 10;
			dig -= d1 * 10;
			*cp++ = d1 + '0';
			*cp++ = dig + '0';
#elif PG_DEC_DIGITS == 1
			if (cp >= endp)
				return -1;
			*cp++ = dig + '0';
#else
#error unsupported NBASE
#endif
			cp = lastp;
		}
	}
	return (int)(cp - buf);
}
