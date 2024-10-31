#include "postgres.h"

#include "access/genam.h"
#include "access/heapam.h"
#include "miscadmin.h"
#include "storage/lmgr.h"
#include "storage/bufmgr.h"
#include "catalog/namespace.h"
#include "utils/lsyscache.h"
#include "utils/builtins.h"
#include <fmgr.h>
#include <funcapi.h>
#include <access/heapam.h>
#include <catalog/pg_type.h>
#include <catalog/heap.h>
#include <commands/vacuum.h>
#include <utils/regproc.h>
#include <utils/varlena.h>


#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

PG_FUNCTION_INFO_V1(fasttruncate);
Datum	fasttruncate(PG_FUNCTION_ARGS);
Datum
fasttruncate(PG_FUNCTION_ARGS) {
	text	*name=PG_GETARG_TEXT_P(0);
	char	*relname;
	List	*relname_list;
	RangeVar	*relvar;
	Oid		relOid;
	Relation	rel;
	bool	make_clean_analyze = false;
	TableScanDesc scan;

	relname = palloc( VARSIZE(name) + 1);
	memcpy(relname, VARDATA(name), VARSIZE(name)-VARHDRSZ);
	relname[ VARSIZE(name)-VARHDRSZ ] = '\0';

	relname_list = stringToQualifiedNameList(relname);
	relvar = makeRangeVarFromNameList(relname_list);
	relOid = RangeVarGetRelid(relvar, AccessExclusiveLock, false);

	if ( get_rel_relkind(relOid) != RELKIND_RELATION )
		elog(ERROR,"Relation isn't a ordinary table");

	rel = table_open(relOid, NoLock);

	if ( !isTempNamespace(get_rel_namespace(relOid)) )
		elog(ERROR,"Relation isn't a temporary table");


	/*
	 * clean table if row count > 0
	*/

	scan = table_beginscan(rel, SnapshotAny, 0, NULL);

	if(heap_getnext(scan,ForwardScanDirection) != NULL) {
		make_clean_analyze = true;;
	}

	table_endscan(scan);

	if( make_clean_analyze ) {
		heap_truncate(list_make1_oid(relOid));
	}

	/*
	 * heap_truncate doesn't unlock the table,
	 * so we should unlock it.
	 */

	table_close(rel, AccessExclusiveLock);

	if ( make_clean_analyze ) {
		VacuumParams	params;
		VacuumRelation	*rel;

		params.options = VACOPT_ANALYZE;
		params.freeze_min_age = -1;
		params.freeze_table_age = -1;
		params.multixact_freeze_min_age = -1;
		params.multixact_freeze_table_age = -1;
		params.is_wraparound = false;
		params.log_min_duration = -1;

		rel = makeNode(VacuumRelation);
		rel->relation = relvar;
		rel->oid = relOid;
		rel->va_cols = NULL;
		vacuum(list_make1(rel), &params,
			   GetAccessStrategy(BAS_VACUUM), false);
	}

	PG_RETURN_VOID();
}
