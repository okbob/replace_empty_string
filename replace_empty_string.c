/*
 * contrib/spi/remove_empty_string.c
 *
 * insert user name in response to a trigger
 * usage:  remove_empty_string (column_name)
 */
#include "postgres.h"

#include "access/htup_details.h"
#include "catalog/pg_type.h"
#include "commands/trigger.h"
#include "executor/spi.h"
#include "miscadmin.h"
#include "parser/parse_coerce.h"
#include "utils/builtins.h"
#include "utils/lsyscache.h"
#include "utils/rel.h"

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(replace_empty_string);

static HeapTuple
__heap_modify_tuple_by_cols(HeapTuple tuple,
						  TupleDesc tupleDesc,
						  int nCols,
						  int *replCols,
						  Datum *replValues,
						  bool *replIsnull)
{
	int			numberOfAttributes = tupleDesc->natts;
	Datum	   *values;
	bool	   *isnull;
	HeapTuple	newTuple;
	int			i;

	/*
	 * allocate and fill values and isnull arrays from the tuple, then replace
	 * selected columns from the input arrays.
	 */
	values = (Datum *) palloc(numberOfAttributes * sizeof(Datum));
	isnull = (bool *) palloc(numberOfAttributes * sizeof(bool));

	heap_deform_tuple(tuple, tupleDesc, values, isnull);

	for (i = 0; i < nCols; i++)
	{
		int			attnum = replCols[i];

		if (attnum <= 0 || attnum > numberOfAttributes)
			elog(ERROR, "invalid column number %d", attnum);
		values[attnum - 1] = replValues[i];
		isnull[attnum - 1] = replIsnull[i];
	}

	/*
	 * create a new tuple from the values and isnull arrays
	 */
	newTuple = heap_form_tuple(tupleDesc, values, isnull);

	pfree(values);
	pfree(isnull);

	/*
	 * copy the identification info of the old tuple: t_ctid, t_self, and OID
	 * (if any)
	 */
	newTuple->t_data->t_ctid = tuple->t_data->t_ctid;
	newTuple->t_self = tuple->t_self;
	newTuple->t_tableOid = tuple->t_tableOid;
	if (tupleDesc->tdhasoid)
		HeapTupleSetOid(newTuple, HeapTupleGetOid(tuple));

	return newTuple;
}

Datum
replace_empty_string(PG_FUNCTION_ARGS)
{
	Trigger		   *trigger;
	TriggerData	   *trigdata = (TriggerData *) fcinfo->context;
	char		   *relname;		/* triggered relation name */
	Relation		rel;			/* triggered relation */
	HeapTuple		rettuple = NULL;
	TupleDesc		tupdesc;		/* tuple description */
	int				attnum;
	int			   *resetcols;
	int				nresetcols = 0;
	Datum		   *values;
	bool		   *nulls;
	bool			warning = false;
	int				nargs;
	char		  **args;

	/* sanity checks from autoinc.c */
	if (!CALLED_AS_TRIGGER(fcinfo))
		/* internal error */
		elog(ERROR, "remove_empty_string: not fired by trigger manager");
	if (!TRIGGER_FIRED_FOR_ROW(trigdata->tg_event))
		/* internal error */
		elog(ERROR, "remove_empty_string: must be fired for row");
	if (!TRIGGER_FIRED_BEFORE(trigdata->tg_event))
		/* internal error */
		elog(ERROR, "remove_empty_string: must be fired before event");

	if (TRIGGER_FIRED_BY_INSERT(trigdata->tg_event))
		rettuple = trigdata->tg_trigtuple;
	else if (TRIGGER_FIRED_BY_UPDATE(trigdata->tg_event))
		rettuple = trigdata->tg_newtuple;
	else
		/* internal error */
		elog(ERROR, "remove_empty_string: cannot process DELETE events");

	rel = trigdata->tg_relation;
	relname = SPI_getrelname(rel);
	tupdesc = rel->rd_att;
	trigger = trigdata->tg_trigger;

	nargs = trigger->tgnargs;
	if (nargs > 0)
	{
		args = trigger->tgargs;
		if (strcmp(args[0], "on") == 0)
			warning = true;
	}
	else
		warning = false;

	resetcols = palloc(tupdesc->natts * sizeof(int));
	nulls = palloc(tupdesc->natts * sizeof(bool));
	values = palloc(tupdesc->natts * sizeof(Datum));

	for (attnum = 1; attnum <= tupdesc->natts; attnum++)
	{
		Oid typid;
		Datum		value;
		bool		isnull;
		TYPCATEGORY category;
		bool		ispreferred;

		typid = SPI_gettypeid(tupdesc, attnum);
		typid = getBaseType(typid);
		get_type_category_preferred(typid, &category, &ispreferred);

		if (category == TYPCATEGORY_STRING)
		{
			value = SPI_getbinval(rettuple, tupdesc, attnum, &isnull);
			if (!isnull)
			{
				text *txt = DatumGetTextP(value);

				if (VARSIZE_ANY_EXHDR(txt) == 0)
				{
					resetcols[nresetcols] = attnum;
					values[nresetcols] = 0;
					nulls[nresetcols++] = true;

					if (warning)
						elog(WARNING,
				"Field \"%s\" of table \"%s\" is empty string (replaced by NULL).",
								SPI_fname(tupdesc, attnum), relname);

				}
			}
		}
	}

	if (nresetcols > 0)
	{
		/* construct new tuple */
		rettuple = __heap_modify_tuple_by_cols(rettuple, tupdesc,
										 nresetcols, resetcols, values, nulls);
	}

	pfree(relname);

	return PointerGetDatum(rettuple);
}
