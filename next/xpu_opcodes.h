/*
 * xpu_opcodes.h
 *
 * collection of built-in xPU opcode
 * ----
 * Copyright 2011-2022 (C) KaiGai Kohei <kaigai@kaigai.gr.jp>
 * Copyright 2014-2022 (C) PG-Strom Developers Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the PostgreSQL License.
 */

/* extension names */
#ifndef EXPR_OPCODE
#define EXPR_OPCODE(NAME)
#endif

/*
 * PostgreSQL Expressions
 */
EXPR_OPCODE(Var)
EXPR_OPCODE(Const)
EXPR_OPCODE(Param)
EXPR_OPCODE(FuncExpr)
EXPR_OPCODE(OpExpr)
EXPR_OPCODE(BoolExpr)

/*
 * PostgreSQL Device Types
 */
#ifndef TYPE_OPCODE
#define TYPE_OPCODE(NAME,OID,EXTENSION)
#endif
TYPE_OPCODE(bool, BOOLOID, NULL)
TYPE_OPCODE(int1, INT1OID, "pg_strom")
TYPE_OPCODE(int2, INT2OID, NULL)
TYPE_OPCODE(int4, INT4OID, NULL)
TYPE_OPCODE(int8, INT8OID, NULL)
TYPE_OPCODE(float2, FLOAT2OID, "pg_strom")
TYPE_OPCODE(float4, FLOAT4OID, NULL)
TYPE_OPCODE(float8, FLOAT8OID, NULL)
TYPE_OPCODE(numeric, NUMERICOID, NULL)
TYPE_OPCODE(bytea, BYTEAOID, NULL)
TYPE_OPCODE(text, TEXTOID, NULL)
//TYPE_OPCODE(varchar, VARCHAROID, NULL)
TYPE_OPCODE(bpchar, BPCHAROID, NULL)
TYPE_OPCODE(date, DATEOID, NULL)
TYPE_OPCODE(time, TIMEOID, NULL)
TYPE_OPCODE(timetz, TIMETZOID, NULL)
TYPE_OPCODE(timestamp, TIMESTAMPOID, NULL)
TYPE_OPCODE(timestamptz, TIMESTAMPTZOID, NULL)
TYPE_OPCODE(interval, INTERVALOID, NULL)
TYPE_OPCODE(money, MONEYOID, NULL)
TYPE_OPCODE(uuid, UUIDOID, NULL)
TYPE_OPCODE(macaddr, MACADDROID, NULL)
TYPE_OPCODE(inet, INETOID, NULL)
//TYPE_OPCODE(cidr, CIDROID, NULL)

/*
 * PostgreSQL Device Functions / Operators
 */
#ifndef FUNC_OPCODE
#define FUNC_OPCODE(FUNC_SQL_NAME,FUNC_ARGS,FUNC_FLAGS,FUNC_DEV_NAME,EXTENSION)
#endif
/* most device functions are sufficient with __FUNC_OPCODE */
#define __FUNC_OPCODE(FUNC_NAME,FUNC_ARGS,EXTENSION)		\
	FUNC_OPCODE(FUNC_NAME,FUNC_ARGS,DEVKERN__ANY,FUNC_NAME,EXTENSION)
#define __FUNC_LOCALE_OPCODE(FUNC_NAME,FUNC_ARGS,EXTENSION)	\
	FUNC_OPCODE(FUNC_NAME,FUNC_ARGS,DEVFUNC__LOCALE_AWARE|DEVKERN__ANY,FUNC_NAME,EXTENSION)

/* type cast functions */
FUNC_OPCODE(bool, int4,   DEVKERN__ANY, int4_to_bool, NULL)
FUNC_OPCODE(int1, int2,   DEVKERN__ANY, int2_to_int1, "pg_strom")
FUNC_OPCODE(int1, int4,   DEVKERN__ANY, int4_to_int1, "pg_strom")
FUNC_OPCODE(int1, int8,   DEVKERN__ANY, int8_to_int1, "pg_strom")
FUNC_OPCODE(int1, float2, DEVKERN__ANY, float2_to_int1, "pg_strom")
FUNC_OPCODE(int1, float4, DEVKERN__ANY, float4_to_int1, "pg_strom")
FUNC_OPCODE(int1, float8, DEVKERN__ANY, float8_to_int1, "pg_strom")

FUNC_OPCODE(int2, int1,   DEVKERN__ANY, int1_to_int2, "pg_strom")
FUNC_OPCODE(int2, int4,   DEVKERN__ANY, int4_to_int2, NULL)
FUNC_OPCODE(int2, int8,   DEVKERN__ANY, int8_to_int2, NULL)
FUNC_OPCODE(int2, float2, DEVKERN__ANY, float2_to_int2, "pg_strom")
FUNC_OPCODE(int2, float4, DEVKERN__ANY, float4_to_int2, NULL)
FUNC_OPCODE(int2, float8, DEVKERN__ANY, float8_to_int2, NULL)

FUNC_OPCODE(int4, int1,   DEVKERN__ANY, int1_to_int4, "pg_strom")
FUNC_OPCODE(int4, int2,   DEVKERN__ANY, int2_to_int4, NULL)
FUNC_OPCODE(int4, int8,   DEVKERN__ANY, int8_to_int4, NULL)
FUNC_OPCODE(int4, float2, DEVKERN__ANY, float2_to_int4, "pg_strom")
FUNC_OPCODE(int4, float4, DEVKERN__ANY, float4_to_int4, NULL)
FUNC_OPCODE(int4, float8, DEVKERN__ANY, float8_to_int4, NULL)

FUNC_OPCODE(int8, int1,   DEVKERN__ANY, int1_to_int8, "pg_strom")
FUNC_OPCODE(int8, int2,   DEVKERN__ANY, int4_to_int8, NULL)
FUNC_OPCODE(int8, int8,   DEVKERN__ANY, int8_to_int8, NULL)
FUNC_OPCODE(int8, float2, DEVKERN__ANY, float2_to_int8, "pg_strom")
FUNC_OPCODE(int8, float4, DEVKERN__ANY, float4_to_int8, NULL)
FUNC_OPCODE(int8, float8, DEVKERN__ANY, float8_to_int8, NULL)

FUNC_OPCODE(float2, int1, DEVKERN__ANY, int1_to_float2, "pg_strom")
FUNC_OPCODE(float2, int2, DEVKERN__ANY, int2_to_float2, "pg_strom")
FUNC_OPCODE(float2, int4, DEVKERN__ANY, int4_to_float2, "pg_strom")
FUNC_OPCODE(float2, int8, DEVKERN__ANY, int8_to_float2, "pg_strom")
FUNC_OPCODE(float2, float4, DEVKERN__ANY, float4_to_float2, "pg_strom")
FUNC_OPCODE(float2, float8, DEVKERN__ANY, float8_to_float2, "pg_strom")

FUNC_OPCODE(float4, int1, DEVKERN__ANY, int1_to_float4, "pg_strom")
FUNC_OPCODE(float4, int2, DEVKERN__ANY, int2_to_float4, NULL)
FUNC_OPCODE(float4, int4, DEVKERN__ANY, int4_to_float4, NULL)
FUNC_OPCODE(float4, int8, DEVKERN__ANY, int8_to_float4, NULL)
FUNC_OPCODE(float4, float2, DEVKERN__ANY, float2_to_float4, "pg_strom")
FUNC_OPCODE(float4, float8, DEVKERN__ANY, float8_to_float4, NULL)

FUNC_OPCODE(float8, int1, DEVKERN__ANY, int1_to_float8, "pg_strom")
FUNC_OPCODE(float8, int2, DEVKERN__ANY, int2_to_float8, NULL)
FUNC_OPCODE(float8, int4, DEVKERN__ANY, int4_to_float8, NULL)
FUNC_OPCODE(float8, int8, DEVKERN__ANY, int8_to_float8, NULL)
FUNC_OPCODE(float8, float2, DEVKERN__ANY, float2_to_float8, "pg_strom")
FUNC_OPCODE(float8, float4, DEVKERN__ANY, float4_to_float8, NULL)

/* '+' : add operators */
__FUNC_OPCODE(int1pl,  int1/int1, "pg_strom")
__FUNC_OPCODE(int12pl, int1/int2, "pg_strom")
__FUNC_OPCODE(int14pl, int1/int4, "pg_strom")
__FUNC_OPCODE(int18pl, int1/int8, "pg_strom")
__FUNC_OPCODE(int21pl, int2/int1, "pg_strom")
__FUNC_OPCODE(int2pl,  int2/int2, NULL)
__FUNC_OPCODE(int24pl, int2/int4, NULL)
__FUNC_OPCODE(int28pl, int2/int8, NULL)
__FUNC_OPCODE(int41pl, int4/int1, "pg_strom")
__FUNC_OPCODE(int42pl, int4/int2, NULL)
__FUNC_OPCODE(int4pl,  int4/int4, NULL)
__FUNC_OPCODE(int48pl, int4/int8, NULL)
__FUNC_OPCODE(int81pl, int8/int1, "pg_strom")
__FUNC_OPCODE(int82pl, int8/int2, NULL)
__FUNC_OPCODE(int84pl, int8/int4, NULL)
__FUNC_OPCODE(int8pl,  int8/int8, NULL)
__FUNC_OPCODE(float2pl,  float2/float2, "pg_strom")
__FUNC_OPCODE(float24pl, float2/float4, "pg_strom")
__FUNC_OPCODE(float28pl, float2/float8, "pg_strom")
__FUNC_OPCODE(float42pl, float4/float2, "pg_strom")
__FUNC_OPCODE(float4pl,  float4/float4, NULL)
__FUNC_OPCODE(float48pl, float4/float8, NULL)
__FUNC_OPCODE(float82pl, float8/float2, "pg_strom")
__FUNC_OPCODE(float84pl, float8/float4, NULL)
__FUNC_OPCODE(float8pl,  float8/float8, NULL)

/* '-' : subtract operators */
__FUNC_OPCODE(int1mi,  int1/int1, "pg_strom")
__FUNC_OPCODE(int12mi, int1/int2, "pg_strom")
__FUNC_OPCODE(int14mi, int1/int4, "pg_strom")
__FUNC_OPCODE(int18mi, int1/int8, "pg_strom")
__FUNC_OPCODE(int21mi, int2/int1, "pg_strom")
__FUNC_OPCODE(int2mi,  int2/int2, NULL)
__FUNC_OPCODE(int24mi, int2/int4, NULL)
__FUNC_OPCODE(int28mi, int2/int8, NULL)
__FUNC_OPCODE(int41mi, int4/int1, "pg_strom")
__FUNC_OPCODE(int42mi, int4/int2, NULL)
__FUNC_OPCODE(int4mi,  int4/int4, NULL)
__FUNC_OPCODE(int48mi, int4/int8, NULL)
__FUNC_OPCODE(int81mi, int8/int1, "pg_strom")
__FUNC_OPCODE(int82mi, int8/int2, NULL)
__FUNC_OPCODE(int84mi, int8/int4, NULL)
__FUNC_OPCODE(int8mi,  int8/int8, NULL)
__FUNC_OPCODE(float2mi,  float2/float2, "pg_strom")
__FUNC_OPCODE(float24mi, float2/float4, "pg_strom")
__FUNC_OPCODE(float28mi, float2/float8, "pg_strom")
__FUNC_OPCODE(float42mi, float4/float2, "pg_strom")
__FUNC_OPCODE(float4mi,  float4/float4, NULL)
__FUNC_OPCODE(float48mi, float4/float8, NULL)
__FUNC_OPCODE(float82mi, float8/float2, "pg_strom")
__FUNC_OPCODE(float84mi, float8/float4, NULL)
__FUNC_OPCODE(float8mi,  float8/float8, NULL)

/* '*' : subtract operators */
__FUNC_OPCODE(int1mul,  int1/int1, "pg_strom")
__FUNC_OPCODE(int12mul, int1/int2, "pg_strom")
__FUNC_OPCODE(int14mul, int1/int4, "pg_strom")
__FUNC_OPCODE(int18mul, int1/int8, "pg_strom")
__FUNC_OPCODE(int21mul, int2/int1, "pg_strom")
__FUNC_OPCODE(int2mul,  int2/int2, NULL)
__FUNC_OPCODE(int24mul, int2/int4, NULL)
__FUNC_OPCODE(int28mul, int2/int8, NULL)
__FUNC_OPCODE(int41mul, int4/int1, "pg_strom")
__FUNC_OPCODE(int42mul, int4/int2, NULL)
__FUNC_OPCODE(int4mul,  int4/int4, NULL)
__FUNC_OPCODE(int48mul, int4/int8, NULL)
__FUNC_OPCODE(int81mul, int8/int1, "pg_strom")
__FUNC_OPCODE(int82mul, int8/int2, NULL)
__FUNC_OPCODE(int84mul, int8/int4, NULL)
__FUNC_OPCODE(int8mul,  int8/int8, NULL)
__FUNC_OPCODE(float2mul,  float2/float2, "pg_strom")
__FUNC_OPCODE(float24mul, float2/float4, "pg_strom")
__FUNC_OPCODE(float28mul, float2/float8, "pg_strom")
__FUNC_OPCODE(float42mul, float4/float2, "pg_strom")
__FUNC_OPCODE(float4mul,  float4/float4, NULL)
__FUNC_OPCODE(float48mul, float4/float8, NULL)
__FUNC_OPCODE(float82mul, float8/float2, "pg_strom")
__FUNC_OPCODE(float84mul, float8/float4, NULL)
__FUNC_OPCODE(float8mul,  float8/float8, NULL)

/* '/' : divide operators */
__FUNC_OPCODE(int1div,  int1/int1, "pg_strom")
__FUNC_OPCODE(int12div, int1/int2, "pg_strom")
__FUNC_OPCODE(int14div, int1/int4, "pg_strom")
__FUNC_OPCODE(int18div, int1/int8, "pg_strom")
__FUNC_OPCODE(int21div, int2/int1, "pg_strom")
__FUNC_OPCODE(int2div,  int2/int2, NULL)
__FUNC_OPCODE(int24div, int2/int4, NULL)
__FUNC_OPCODE(int28div, int2/int8, NULL)
__FUNC_OPCODE(int41div, int4/int1, "pg_strom")
__FUNC_OPCODE(int42div, int4/int2, NULL)
__FUNC_OPCODE(int4div,  int4/int4, NULL)
__FUNC_OPCODE(int48div, int4/int8, NULL)
__FUNC_OPCODE(int81div, int8/int1, "pg_strom")
__FUNC_OPCODE(int82div, int8/int2, NULL)
__FUNC_OPCODE(int84div, int8/int4, NULL)
__FUNC_OPCODE(int8div,  int8/int8, NULL)
__FUNC_OPCODE(float2div,  float2/float2, "pg_strom")
__FUNC_OPCODE(float24div, float2/float4, "pg_strom")
__FUNC_OPCODE(float28div, float2/float8, "pg_strom")
__FUNC_OPCODE(float42div, float4/float2, "pg_strom")
__FUNC_OPCODE(float4div,  float4/float4, NULL)
__FUNC_OPCODE(float48div, float4/float8, NULL)
__FUNC_OPCODE(float82div, float8/float2, "pg_strom")
__FUNC_OPCODE(float84div, float8/float4, NULL)
__FUNC_OPCODE(float8div,  float8/float8, NULL)

/* '%' : reminder operators */
__FUNC_OPCODE(int1mod, int1/int1, "pg_strom")
__FUNC_OPCODE(int2mod, int2/int2, NULL)
__FUNC_OPCODE(int4mod, int4/int4, NULL)
__FUNC_OPCODE(int8mod, int8/int8, NULL)

/* '+' : unary plus operators */
__FUNC_OPCODE(int1up, int1, "pg_strom")
__FUNC_OPCODE(int2up, int2, NULL)
__FUNC_OPCODE(int4up, int2, NULL)
__FUNC_OPCODE(int8up, int2, NULL)
__FUNC_OPCODE(float2up, float2, "pg_strom")
__FUNC_OPCODE(float4up, float4, NULL)
__FUNC_OPCODE(float8up, float8, NULL)

/* '-' : unary minus operators */
__FUNC_OPCODE(int1um, int1, "pg_strom")
__FUNC_OPCODE(int2um, int2, NULL)
__FUNC_OPCODE(int4um, int2, NULL)
__FUNC_OPCODE(int8um, int2, NULL)
__FUNC_OPCODE(float2um, float2, "pg_strom")
__FUNC_OPCODE(float4um, float4, NULL)
__FUNC_OPCODE(float8um, float8, NULL)

/* '@' : absolute value operators */
__FUNC_OPCODE(int1abs, int1, "pg_strom")
__FUNC_OPCODE(int2abs, int2, NULL)
__FUNC_OPCODE(int4abs, int2, NULL)
__FUNC_OPCODE(int8abs, int2, NULL)
__FUNC_OPCODE(float2abs, float2, "pg_strom")
__FUNC_OPCODE(float4abs, float4, NULL)
__FUNC_OPCODE(float8abs, float8, NULL)

/* '=' : equal operators */
__FUNC_OPCODE(booleq,  bool/bool, NULL)
__FUNC_OPCODE(int1eq,  int1/int1, "pg_strom")
__FUNC_OPCODE(int12eq, int1/int2, "pg_strom")
__FUNC_OPCODE(int14eq, int1/int4, "pg_strom")
__FUNC_OPCODE(int18eq, int1/int8, "pg_strom")
__FUNC_OPCODE(int21eq, int2/int1, "pg_strom")
__FUNC_OPCODE(int2eq,  int2/int2, NULL)
__FUNC_OPCODE(int24eq, int2/int4, NULL)
__FUNC_OPCODE(int28eq, int2/int8, NULL)
__FUNC_OPCODE(int41eq, int4/int1, "pg_strom")
__FUNC_OPCODE(int42eq, int4/int2, NULL)
__FUNC_OPCODE(int4eq,  int4/int4, NULL)
__FUNC_OPCODE(int48eq, int4/int8, NULL)
__FUNC_OPCODE(int81eq, int8/int1, "pg_strom")
__FUNC_OPCODE(int82eq, int8/int2, NULL)
__FUNC_OPCODE(int84eq, int8/int4, NULL)
__FUNC_OPCODE(int8eq,  int8/int8, NULL)
__FUNC_OPCODE(float2eq,  float2/float2, "pg_strom")
__FUNC_OPCODE(float24eq, float2/float4, "pg_strom")
__FUNC_OPCODE(float28eq, float2/float8, "pg_strom")
__FUNC_OPCODE(float42eq, float4/float2, "pg_strom")
__FUNC_OPCODE(float4eq,  float4/float4, NULL)
__FUNC_OPCODE(float48eq, float4/float8, NULL)
__FUNC_OPCODE(float82eq, float8/float2, "pg_strom")
__FUNC_OPCODE(float84eq, float8/float4, NULL)
__FUNC_OPCODE(float8eq,  float8/float8, NULL)

/* '<>' : not equal operators */
__FUNC_OPCODE(int1ne,  int1/int1, "pg_strom")
__FUNC_OPCODE(int12ne, int1/int2, "pg_strom")
__FUNC_OPCODE(int14ne, int1/int4, "pg_strom")
__FUNC_OPCODE(int18ne, int1/int8, "pg_strom")
__FUNC_OPCODE(int21ne, int2/int1, "pg_strom")
__FUNC_OPCODE(int2ne,  int2/int2, NULL)
__FUNC_OPCODE(int24ne, int2/int4, NULL)
__FUNC_OPCODE(int28ne, int2/int8, NULL)
__FUNC_OPCODE(int41ne, int4/int1, "pg_strom")
__FUNC_OPCODE(int42ne, int4/int2, NULL)
__FUNC_OPCODE(int4ne,  int4/int4, NULL)
__FUNC_OPCODE(int48ne, int4/int8, NULL)
__FUNC_OPCODE(int81ne, int8/int1, "pg_strom")
__FUNC_OPCODE(int82ne, int8/int2, NULL)
__FUNC_OPCODE(int84ne, int8/int4, NULL)
__FUNC_OPCODE(int8ne,  int8/int8, NULL)
__FUNC_OPCODE(float2ne,  float2/float2, "pg_strom")
__FUNC_OPCODE(float24ne, float2/float4, "pg_strom")
__FUNC_OPCODE(float28ne, float2/float8, "pg_strom")
__FUNC_OPCODE(float42ne, float4/float2, "pg_strom")
__FUNC_OPCODE(float4ne,  float4/float4, NULL)
__FUNC_OPCODE(float48ne, float4/float8, NULL)
__FUNC_OPCODE(float82ne, float8/float2, "pg_strom")
__FUNC_OPCODE(float84ne, float8/float4, NULL)
__FUNC_OPCODE(float8ne,  float8/float8, NULL)

/* '>' : greater than operators */
__FUNC_OPCODE(int1gt,  int1/int1, "pg_strom")
__FUNC_OPCODE(int12gt, int1/int2, "pg_strom")
__FUNC_OPCODE(int14gt, int1/int4, "pg_strom")
__FUNC_OPCODE(int18gt, int1/int8, "pg_strom")
__FUNC_OPCODE(int21gt, int2/int1, "pg_strom")
__FUNC_OPCODE(int2gt,  int2/int2, NULL)
__FUNC_OPCODE(int24gt, int2/int4, NULL)
__FUNC_OPCODE(int28gt, int2/int8, NULL)
__FUNC_OPCODE(int41gt, int4/int1, "pg_strom")
__FUNC_OPCODE(int42gt, int4/int2, NULL)
__FUNC_OPCODE(int4gt,  int4/int4, NULL)
__FUNC_OPCODE(int48gt, int4/int8, NULL)
__FUNC_OPCODE(int81gt, int8/int1, "pg_strom")
__FUNC_OPCODE(int82gt, int8/int2, NULL)
__FUNC_OPCODE(int84gt, int8/int4, NULL)
__FUNC_OPCODE(int8gt,  int8/int8, NULL)
__FUNC_OPCODE(float2gt,  float2/float2, "pg_strom")
__FUNC_OPCODE(float24gt, float2/float4, "pg_strom")
__FUNC_OPCODE(float28gt, float2/float8, "pg_strom")
__FUNC_OPCODE(float42gt, float4/float2, "pg_strom")
__FUNC_OPCODE(float4gt,  float4/float4, NULL)
__FUNC_OPCODE(float48gt, float4/float8, NULL)
__FUNC_OPCODE(float82gt, float8/float2, "pg_strom")
__FUNC_OPCODE(float84gt, float8/float4, NULL)
__FUNC_OPCODE(float8gt,  float8/float8, NULL)

/* '<' : less than operators */
__FUNC_OPCODE(int1lt,  int1/int1, "pg_strom")
__FUNC_OPCODE(int12lt, int1/int2, "pg_strom")
__FUNC_OPCODE(int14lt, int1/int4, "pg_strom")
__FUNC_OPCODE(int18lt, int1/int8, "pg_strom")
__FUNC_OPCODE(int21lt, int2/int1, "pg_strom")
__FUNC_OPCODE(int2lt,  int2/int2, NULL)
__FUNC_OPCODE(int24lt, int2/int4, NULL)
__FUNC_OPCODE(int28lt, int2/int8, NULL)
__FUNC_OPCODE(int41lt, int4/int1, "pg_strom")
__FUNC_OPCODE(int42lt, int4/int2, NULL)
__FUNC_OPCODE(int4lt,  int4/int4, NULL)
__FUNC_OPCODE(int48lt, int4/int8, NULL)
__FUNC_OPCODE(int81lt, int8/int1, "pg_strom")
__FUNC_OPCODE(int82lt, int8/int2, NULL)
__FUNC_OPCODE(int84lt, int8/int4, NULL)
__FUNC_OPCODE(int8lt,  int8/int8, NULL)
__FUNC_OPCODE(float2lt,  float2/float2, "pg_strom")
__FUNC_OPCODE(float24lt, float2/float4, "pg_strom")
__FUNC_OPCODE(float28lt, float2/float8, "pg_strom")
__FUNC_OPCODE(float42lt, float4/float2, "pg_strom")
__FUNC_OPCODE(float4lt,  float4/float4, NULL)
__FUNC_OPCODE(float48lt, float4/float8, NULL)
__FUNC_OPCODE(float82lt, float8/float2, "pg_strom")
__FUNC_OPCODE(float84lt, float8/float4, NULL)
__FUNC_OPCODE(float8lt,  float8/float8, NULL)

/* '>=' : relational greater-than or equal-to */	
__FUNC_OPCODE(int1ge,  int1/int1, "pg_strom")
__FUNC_OPCODE(int12ge, int1/int2, "pg_strom")
__FUNC_OPCODE(int14ge, int1/int4, "pg_strom")
__FUNC_OPCODE(int18ge, int1/int8, "pg_strom")
__FUNC_OPCODE(int21ge, int2/int1, "pg_strom")
__FUNC_OPCODE(int2ge,  int2/int2, NULL)
__FUNC_OPCODE(int24ge, int2/int4, NULL)
__FUNC_OPCODE(int28ge, int2/int8, NULL)
__FUNC_OPCODE(int41ge, int4/int1, "pg_strom")
__FUNC_OPCODE(int42ge, int4/int2, NULL)
__FUNC_OPCODE(int4ge,  int4/int4, NULL)
__FUNC_OPCODE(int48ge, int4/int8, NULL)
__FUNC_OPCODE(int81ge, int8/int1, "pg_strom")
__FUNC_OPCODE(int82ge, int8/int2, NULL)
__FUNC_OPCODE(int84ge, int8/int4, NULL)
__FUNC_OPCODE(int8ge,  int8/int8, NULL)
__FUNC_OPCODE(float2ge,  float2/float2, "pg_strom")
__FUNC_OPCODE(float24ge, float2/float4, "pg_strom")
__FUNC_OPCODE(float28ge, float2/float8, "pg_strom")
__FUNC_OPCODE(float42ge, float4/float2, "pg_strom")
__FUNC_OPCODE(float4ge,  float4/float4, NULL)
__FUNC_OPCODE(float48ge, float4/float8, NULL)
__FUNC_OPCODE(float82ge, float8/float2, "pg_strom")
__FUNC_OPCODE(float84ge, float8/float4, NULL)
__FUNC_OPCODE(float8ge,  float8/float8, NULL)

/* '<=' : relational less-than or equal-to */
__FUNC_OPCODE(int1le,  int1/int1, "pg_strom")
__FUNC_OPCODE(int12le, int1/int2, "pg_strom")
__FUNC_OPCODE(int14le, int1/int4, "pg_strom")
__FUNC_OPCODE(int18le, int1/int8, "pg_strom")
__FUNC_OPCODE(int21le, int2/int1, "pg_strom")
__FUNC_OPCODE(int2le,  int2/int2, NULL)
__FUNC_OPCODE(int24le, int2/int4, NULL)
__FUNC_OPCODE(int28le, int2/int8, NULL)
__FUNC_OPCODE(int41le, int4/int1, "pg_strom")
__FUNC_OPCODE(int42le, int4/int2, NULL)
__FUNC_OPCODE(int4le,  int4/int4, NULL)
__FUNC_OPCODE(int48le, int4/int8, NULL)
__FUNC_OPCODE(int81le, int8/int1, "pg_strom")
__FUNC_OPCODE(int82le, int8/int2, NULL)
__FUNC_OPCODE(int84le, int8/int4, NULL)
__FUNC_OPCODE(int8le,  int8/int8, NULL)
__FUNC_OPCODE(float2le,  float2/float2, "pg_strom")
__FUNC_OPCODE(float24le, float2/float4, "pg_strom")
__FUNC_OPCODE(float28le, float2/float8, "pg_strom")
__FUNC_OPCODE(float42le, float4/float2, "pg_strom")
__FUNC_OPCODE(float4le,  float4/float4, NULL)
__FUNC_OPCODE(float48le, float4/float8, NULL)
__FUNC_OPCODE(float82le, float8/float2, "pg_strom")
__FUNC_OPCODE(float84le, float8/float4, NULL)
__FUNC_OPCODE(float8le,  float8/float8, NULL)

/* '&' : bitwise and */
__FUNC_OPCODE(int1and, int1/int1, "pg_strom")
__FUNC_OPCODE(int2and, int2/int2, NULL)
__FUNC_OPCODE(int4and, int4/int4, NULL)
__FUNC_OPCODE(int8and, int8/int8, NULL)

/* '|'  : bitwise or */
__FUNC_OPCODE(int1or, int1/int1, "pg_strom")
__FUNC_OPCODE(int2or, int2/int2, NULL)
__FUNC_OPCODE(int4or, int4/int4, NULL)
__FUNC_OPCODE(int8or, int8/int8, NULL)

/* '#'  : bitwise xor */
__FUNC_OPCODE(int1xor, int1/int1, "pg_strom")
__FUNC_OPCODE(int2xor, int2/int2, NULL)
__FUNC_OPCODE(int4xor, int4/int4, NULL)
__FUNC_OPCODE(int8xor, int8/int8, NULL)	
	
/* '~'  : bitwise not */
__FUNC_OPCODE(int1not, int1, "pg_strom")
__FUNC_OPCODE(int2not, int2, NULL)
__FUNC_OPCODE(int4not, int4, NULL)
__FUNC_OPCODE(int8not, int8, NULL)	

/* '>>' : right shift */
__FUNC_OPCODE(int1shr, int1/int4, "pg_strom")
__FUNC_OPCODE(int2shr, int2/int4, NULL)
__FUNC_OPCODE(int4shr, int4/int4, NULL)
__FUNC_OPCODE(int8shr, int8/int4, NULL)

/* '<<' : left shift */
__FUNC_OPCODE(int1shl, int1/int4, "pg_strom")
__FUNC_OPCODE(int2shl, int2/int4, NULL)
__FUNC_OPCODE(int4shl, int4/int4, NULL)
__FUNC_OPCODE(int8shl, int8/int4, NULL)

/* LIKE operators */
__FUNC_OPCODE(like, text/text, NULL)
__FUNC_OPCODE(textlike, text/text, NULL)
__FUNC_OPCODE(bpcharlike, bpchar/text, NULL)
__FUNC_OPCODE(notlike, text/text, NULL)
__FUNC_OPCODE(textnlike, text/text, NULL)
__FUNC_OPCODE(bpcharnlike, bpchar/text, NULL)	
__FUNC_LOCALE_OPCODE(texticlike, text/text, NULL)
__FUNC_LOCALE_OPCODE(bpchariclike, bpchar/text, NULL)
__FUNC_LOCALE_OPCODE(texticnlike, text/text, NULL)
__FUNC_LOCALE_OPCODE(bpcharicnlike, bpchar/text, NULL)

/* String operations */
__FUNC_OPCODE(textcat, text/text, NULL)
__FUNC_OPCODE(concat, __text__, NULL)

#undef EXPR_OPCODE
#undef TYPE_OPCODE
#undef FUNC_OPCODE
