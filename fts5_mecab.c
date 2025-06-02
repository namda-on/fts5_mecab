/*
** ** Implementation Help **
** https://www.sqlite.org/loadext.html
** 4. Programming Loadable Extensions
**   A template loadable extension contains the following three elements:
**     1. Use "#include <sqlite3ext.h>" at the top of your source code files instead of "#include <sqlite3.h>".
**     2. Put the macro "SQLITE_EXTENSION_INIT1" on a line by itself right after the "#include <sqlite3ext.h>" line.
*/

#include <sqlite3ext.h>
SQLITE_EXTENSION_INIT1
#include <assert.h>
#include <mecab.h>
#include <stdlib.h>
#include <string.h>

/*
** ** Implementation Help **
** https://www.sqlite.org/fts5.html
** 7. Extending FTS5
**   Before a new auxiliary function or tokenizer implementation may 
**   be registered with FTS5, an application must obtain a pointer
**   to the "fts5_api" structure. ... The following example code 
**   demonstrates the technique: 
*/

/*
** Return a pointer to the fts5_api pointer for database connection db.
** If an error occurs, return NULL and leave an error in the database 
** handle (accessible using sqlite3_errcode()/errmsg()).
*/
fts5_api *fts5_api_from_db(sqlite3 *db){
  fts5_api *pRet = 0;
  sqlite3_stmt *pStmt = 0;

  /*
  ** ** Implementation Help **
  ** https://www.sqlite.org/c3ref/bind_blob.html
  ** ...
  ** The second argument is the index of the SQL parameter to be
  **  set. The leftmost SQL parameter has an index of 1.
  ** ...
  ** sqlite3_bind_pointer(S,I,P,T,D) → Bind pointer P of type T to
  ** the I-th parameter of prepared statement S. D is an optional
  ** destructor function for P. 
  */
  if( SQLITE_OK==sqlite3_prepare(db, "SELECT fts5(?1)", -1, &pStmt, 0) ){
    sqlite3_bind_pointer(pStmt, 1, (void*)&pRet, "fts5_api_ptr", NULL);
    sqlite3_step(pStmt);
  }
  sqlite3_finalize(pStmt);
  return pRet;
}

/*
** ** Implementation Help **
** https://www.sqlite.org/fts5.html
** 7. Extending FTS5
**   7.1. Custom Tokenizers
**     To create a custom tokenizer, an application must implement
**     three functions: a tokenizer constructor (xCreate), 
**     a destructor (xDelete) and a function to do the actual 
**     tokenization (xTokenize). ...
**
**     typedef struct Fts5Tokenizer Fts5Tokenizer;
**     typedef struct fts5_tokenizer fts5_tokenizer;
**
*/


static char *global_dict_path = NULL;
static char *global_rc_path = NULL;
static void mecab_dict(sqlite3_context *pCtx, int nVal, sqlite3_value **apVal){
#ifdef DEBUG
    printf("mecab_dict()\n");
#endif
  if (nVal >= 1){
    const char *text = (const char *) sqlite3_value_text(apVal[0]);
    if (text) {
      size_t len = strlen(text);
      char sep = '/';
      char *new_path = (char *)sqlite3_malloc(len + 2);
      if (new_path == NULL) {
        sqlite3_result_error_nomem(pCtx);
        return;
      }
      strcpy(new_path, text);
      if (len > 0 && new_path[len - 1] != sep) {
        new_path[len] = sep;
        new_path[len + 1] = '\0';
      } else {
        new_path[len] = '\0'; // 널 종료 문자 보장
      }

      if (global_dict_path != NULL) {
        sqlite3_free(global_dict_path);
      }
      global_dict_path = new_path;
#ifdef DEBUG
      printf("new dict_path: %s\n", new_path);
#endif
      sqlite3_result_text(pCtx, global_dict_path, -1, SQLITE_TRANSIENT);
      return;
    }
  }
  sqlite3_result_null(pCtx);
}

static void mecab_rc(sqlite3_context *pCtx, int nVal, sqlite3_value **apVal){
#ifdef DEBUG
    printf("mecab_rc()\n");
#endif
    if (nVal >= 1){
        const char *text = (const char *) sqlite3_value_text(apVal[0]);
        if (text) {
            // No need to append slash here, it's a file path
            char *new_rc_path = (char *)sqlite3_malloc(strlen(text) + 1);
            if (new_rc_path == NULL) {
                sqlite3_result_error_nomem(pCtx);
                return;
            }
            strcpy(new_rc_path, text);

            if (global_rc_path != NULL) {
                sqlite3_free(global_rc_path);
            }
            global_rc_path = new_rc_path;
#ifdef DEBUG
            printf("new rc_path: %s\n", global_rc_path);
#endif
            sqlite3_result_text(pCtx, global_rc_path, -1, SQLITE_TRANSIENT);
            return;
        }
    }
    sqlite3_result_null(pCtx);
}

/*
** Implementation Help
** https://taku910.github.io/mecab/libmecab.html
*/
typedef struct MecabTokenizer {
  fts5_tokenizer base;
  mecab_t *mecab;
  int verbose;
  int stop789;
} MecabTokenizer;

static int mecabCreate(
  void *pContext,
  const char **azArg, int nArg,
  Fts5Tokenizer **ppOut
){
  fts5_api *pApi = (fts5_api*)pContext;
  MecabTokenizer *p = NULL;
  int rc = SQLITE_OK;

  p = sqlite3_malloc(sizeof(MecabTokenizer));
  if (p == NULL) {
    return SQLITE_NOMEM;
  }
  memset(p, 0, sizeof(MecabTokenizer));

  for (int i = 0; i < nArg; i++) {
    if (strcmp(azArg[i], "stop789") == 0) {
      p->stop789 = 1;
    } else if (strcmp(azArg[i], "vv") == 0) {
      p->verbose = 2;
    } else if (strcmp(azArg[i], "v") == 0) {
      p->verbose = 1;
    }
  }
#ifdef DEBUG
  if (p->verbose > 0) {
    printf("mecabCreate(): verbose = %d, stop789 = %d\n", p->verbose, p->stop789);
  }
#endif

  const int MAX_MECAB_ARGS = 1 + 2 + 2 + nArg; // "program_name" + "-d" + "dict_path" + "-r" + "rcfile" + nArg;
  char *mecab_argv_buffer[MAX_MECAB_ARGS];
  int mecab_argc = 0;
  mecab_argv_buffer[mecab_argc++] = "mecab_tokenizer_instance";
  if (global_dict_path != NULL) {
    mecab_argv_buffer[mecab_argc++] = "-d";
    mecab_argv_buffer[mecab_argc++] = global_dict_path;
#ifdef DEBUG
    if (p -> verbose > 0) {
      printf("dict path : %s\n", global_dict_path);
    }
#endif
  }
  if (global_rc_path != NULL) {
    mecab_argv_buffer[mecab_argc++] = "-r";
    mecab_argv_buffer[mecab_argc++] = global_rc_path;
#ifdef DEBUG
    if (p -> verbose > 0) {
      printf("rc path : %s\n", global_rc_path);
    }
#endif
  }

  // FTS5의 CREATE VIRTUAL TABLE 구문에서 `tokenize='mecab "arg1" "arg2"'` 형태로 넘어온 인자들
  for (int i = 0; i < nArg; ++i) {
      mecab_argv_buffer[mecab_argc++] = (char*)azArg[i];
  }

#ifdef DEBUG
  if (p -> verbose > 0) { // DEBUG LEVEL 1
    printf("Args passed to mecab_new:\n");
    for (int i = 0; i < mecab_argc; ++i) {
      printf("  Arg %d: %s\n", i, mecab_argv_buffer[i]);
    }
  }
#endif
  p->mecab = mecab_new(mecab_argc, mecab_argv_buffer);
  if (p->mecab == NULL) {
    sqlite3_free(p);
    return SQLITE_ERROR;
  }
#ifdef DEBUG
  /*
  ** ** Implementation Help **
  ** https://taku910.github.io/mecab/libmecab.html
  */
  // Dictionary info
  if (p -> verbose > 0) { // DEBUG LEVEL 1
    const mecab_dictionary_info_t *d = mecab_dictionary_info(p->mecab);
    for (; d; d = d->next) {
      printf("mecab_dictionary_info()\n");
      printf("  filename: %s\n", d->filename);
      printf("  charset: %s\n", d->charset);
      printf("  size: %d\n", d->size);
      printf("  type: %d\n", d->type);
      printf("  lsize: %d\n", d->lsize);
      printf("  rsize: %d\n", d->rsize);
      printf("  version: %d\n", d->version);
    }
  }
#endif

  *ppOut = (Fts5Tokenizer*)p;
  return SQLITE_OK;
}

static void mecabDelete(Fts5Tokenizer *pTokenizer){
  MecabTokenizer *p = (MecabTokenizer*)pTokenizer;
#ifdef DEBUG
  if (p->verbose > 0) { // DEBUG LEVEL 1
    printf("mecabDelete()\n");
  }
#endif
  /*
  ** Implementation Help
  ** https://taku910.github.io/mecab/libmecab.html
  */
  if (p -> mecab) {
    mecab_destroy(p->mecab);
  }
  p->verbose = 0;
  p->stop789 = 0;
  sqlite3_free(p);
}

static int mecabTokenize(
  Fts5Tokenizer *pTokenizer, 
  void *pCtx,
  int flags,            /* Mask of FTS5_TOKENIZE_* flags */
  const char *pText, int nText, 
  int (*xToken)(
    void *pCtx,         /* Copy of 2nd argument to xTokenize() */
    int tflags,         /* Mask of FTS5_TOKEN_* flags */
    const char *pToken, /* Pointer to buffer containing token */
    int nToken,         /* Size of token in bytes */
    int iStart,         /* Byte offset of token within input text */
    int iEnd            /* Byte offset of end of token within input text */
  )
){
  MecabTokenizer *p = (MecabTokenizer*)pTokenizer;
#ifdef DEBUG
  if (p->verbose > 0) { // DEBUG LEVEL 1
    printf("mecabTokenize()\n");
  }
#endif

  const mecab_node_t *node;
  int nlen;
  char *tmp;
  char *buf;
  int buflen;
  int offset;
  int rc;
  
#ifdef DEBUG
  if (p->verbose > 0) { // DEBUG LEVEL 1
    printf("pText (nText) = %s (%d)\n", pText, nText);
  }
#endif

  /* parse */
  node = mecab_sparse_tonode2(p->mecab, pText, strlen(pText)+1);
  if (node == NULL) {
    return SQLITE_ERROR;
  }
  
  /* initialize */
  nlen = 0;
  #define DEFAULT_BUFFER_LENGTH 256
  buf = malloc(DEFAULT_BUFFER_LENGTH);
  if(buf == NULL){
    return SQLITE_NOMEM;
  }
  buflen = DEFAULT_BUFFER_LENGTH;
  offset = 0;
  rc = SQLITE_OK;

#ifdef DEBUG
  int _node_count = 0;  // for DEBUG
  int _token_count = 0; // for DEBUG
#endif
  
  while (node != NULL) {
    while (node->next != NULL && node->length == 0) {

#ifdef DEBUG
      _node_count += 1;
      if (p->verbose > 0) { // DEBUG LEVEL 1
        printf("increment _node_count [1]: %s\n", node->feature);
      }
#endif
      offset += node->rlength;
      node = node->next;
    }

#ifdef DEBUG
    if (p->verbose > 1) { // DEBUG LEVEL 2
      // printf("pText (nText) = %s (%d)\n", pText, nText);
      printf("node info\n");
      printf("  feature     = %s\n", node->feature);
      printf("  surface     = %s\n", node->surface);
      printf("  length      = %d\n", node->length);
      printf("  rlength     = %d\n", node->rlength);
      printf("  posid       = %d\n", node->posid);
      printf("  char_type   = %d\n", node->char_type);
      printf("  stat        = %d\n", node->stat);
      printf("--------------\n");
    }
#endif

    nlen = node->length;
    offset += node->rlength - nlen;
    if (nlen > buflen) {
      tmp = (char *)realloc(buf, nlen + 1);
      if(tmp == NULL) {
        rc = SQLITE_NOMEM;
        break;
      }else{
        buf = tmp;
      }
      buf[nlen] = '\0';
      buflen = nlen;
    }
    strncpy(buf, node->surface, nlen);
    buf[nlen] = '\0';

#ifdef DEBUG
    if (p->verbose > 1) { // DEBUG LEVEL 2
      printf("calling xToken()\n");
      printf("  tflags      = 0\n");
      printf("  pToken      = %s\n", buf);
      printf("  nToken      = %d\n", nlen);
      printf("  iStart      = %d\n", offset);
      printf("  iEnd        = %d\n", offset + nlen);
      printf("==============\n");
    }
#endif

#ifdef STOP789
    if (!p->stop789 || node->posid > 9 || node->posid < 7) {
      rc = xToken(pCtx, 0, buf, nlen, offset, offset + nlen);
    #ifdef DEBUG
      _token_count += 1;
    } else {
      if (p->verbose > 0) { // DEBUG LEVEL 1
        printf("increment _node_count [3]: %s\n", node->feature);
      }
    #endif
    }
#else
    rc = xToken(pCtx, 0, buf, nlen, offset, offset + nlen);
    #ifdef DEBUG
    _token_count += 1;
    #endif
#endif

    if (rc != SQLITE_OK) {
      /*
      ** If an xToken() callback returns any value other than
      ** SQLITE_OK, then the tokenization should be abandoned
      ** and the xTokenize() method should immediately return
      ** a copy of the xToken() return value. 
      */

#ifdef DEBUG
      if (p->verbose > 0) { // DEBUG LEVEL 1
        printf("break [1]: xToken() rc = %d\n", rc);
      }
#endif

      break;
    }
    offset += node->length;

#ifdef DEBUG
    _node_count += 1;
#endif

    node = node->next;
    if (offset >= nText) {
      rc = SQLITE_OK;  // SQLITE_DONE;

#ifdef DEBUG
      if (p->verbose > 0) { // DEBUG LEVEL 1
        printf("break [2]: offset >= nText\n");
      }
#endif

      break;
    }
  }
  
  /* clean up */
  while (node != NULL) {

#ifdef DEBUG
    _node_count += 1;
    if (p->verbose > 0) { // DEBUG LEVEL 1
      printf("increment _node_count [2]: %s\n", node->feature);
    }
#endif

    node = node->next;
  }
  nlen = 0;
  if (buf) {
    free(buf);
  }
  buflen = 0;
  offset = 0;

#ifdef DEBUG
  if (p->verbose > 0) { // DEBUG LEVEL 1
    printf("_node_count, _token_count = %d, %d\n", _node_count, _token_count);
  }
#endif
  
  return rc;
}


#ifdef _WIN32
__declspec(dllexport)
#endif
int sqlite3_ftsmecab_init( /* entry point for "fts5_mecab.o" */ 
  sqlite3 *db, 
  char **pzErrMsg, 
  const sqlite3_api_routines *pApi
){
  int rc = SQLITE_OK;
  SQLITE_EXTENSION_INIT2(pApi);

  fts5_api *pApi_fts5;
  
  pApi_fts5 = fts5_api_from_db(db);
  if( pApi_fts5==0 ){
    *pzErrMsg = sqlite3_mprintf("fts5_api_from_db: %s", sqlite3_errmsg(db));
    return SQLITE_ERROR;
  }
  
  fts5_tokenizer t;

  rc = sqlite3_create_function(db, "mecab_dict", 1, SQLITE_UTF8 | SQLITE_DETERMINISTIC, NULL, &mecab_dict, NULL, NULL);
  rc = sqlite3_create_function(db, "mecab_rc", 1, SQLITE_UTF8 | SQLITE_DETERMINISTIC, NULL, &mecab_rc, NULL, NULL);
  
  t.xCreate = mecabCreate;
  t.xDelete = mecabDelete;
  t.xTokenize = mecabTokenize;
  rc = pApi_fts5->xCreateTokenizer(pApi_fts5, "mecab", (void*)pApi_fts5, &t, 0);
  return rc;
}
