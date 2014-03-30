#include "dbfs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static
DBFS_Blob slurp(FILE *in)
{
    uint8_t *buf = NULL;
    size_t size = 0, cap = 0;

    cap = 4096;
    buf = malloc(cap);
    while (true)
    {
        size_t got;
        if (size == cap)
        {
            cap *= 2;
            buf = realloc(buf, cap);
        }
        got = fread(buf + size, 1, cap - size, in);
        if (!got)
            break;
        size += got;
    }
    buf = realloc(buf, size);
    return (DBFS_Blob){buf, size};
}

static
int do_mkd(DBFS *dbfs, const char *dname)
{
    if (!dname)
    {
        puts("missing argument");
        return 1;
    }
    if (DBFS_OKAY != dbfs_mkd(dbfs, (DBFS_DirName){dname}))
    {
        puts("not okay");
        return 2;
    }
    return 0;
}

static
int do_rmd(DBFS *dbfs, const char *dname)
{
    if (!dname)
    {
        puts("missing argument");
        return 1;
    }
    if (DBFS_OKAY != dbfs_rmd(dbfs, (DBFS_DirName){dname}))
    {
        puts("not okay");
        return 2;
    }
    return 0;
}

static
int do_mvd(DBFS *dbfs, const char *from, const char *to)
{
    if (!from || !to)
    {
        puts("missing argument");
        return 1;
    }
    if (DBFS_OKAY != dbfs_mvd(dbfs, (DBFS_DirName){from}, (DBFS_DirName){to}))
    {
        puts("not okay");
        return 2;
    }
    return 0;
}

static
int do_lsd(DBFS *dbfs, const char *fname, FILE *out)
{
    DBFS_DirList dlist;
    size_t i;
    if (!fname)
    {
        puts("missing argument");
        return 1;
    }
    if (DBFS_OKAY != dbfs_lsd(dbfs, (DBFS_DirName){fname}, &dlist))
    {
        puts("not okay");
        return 2;
    }
    fprintf(out, "%zu directories:\n", dlist.count);
    for (i = 0; i < dlist.count; ++i)
        fprintf(out, "  [%zu]: %s\n", i, dlist.dirs[i].name);
    dbfs_free_dir_list(dlist);
    return 0;
}

static
int do_lsf(DBFS *dbfs, const char *fname, FILE *out)
{
    DBFS_FileList flist;
    size_t i;
    if (!fname)
    {
        puts("missing argument");
        return 1;
    }
    if (DBFS_OKAY != dbfs_lsf(dbfs, (DBFS_DirName){fname}, &flist))
    {
        puts("not okay");
        return 2;
    }
    fprintf(out, "%zu files:\n", flist.count);
    for (i = 0; i < flist.count; ++i)
        fprintf(out, "  [%zu]: %s\n", i, flist.files[i].name);
    dbfs_free_file_list(flist);
    return 0;
}

static
int do_get(DBFS *dbfs, const char *fname, FILE *out)
{
    DBFS_Blob blob;
    if (!fname)
    {
        puts("missing argument");
        return 1;
    }
    if (DBFS_OKAY != dbfs_get(dbfs, (DBFS_FileName){fname}, &blob))
    {
        puts("not okay");
        return 2;
    }
    fwrite(blob.data, 1, blob.size, out);
    dbfs_free_blob(blob);
    return 0;
}

static
int do_put(DBFS *dbfs, const char *fname, FILE *in)
{
    DBFS_Blob blob;
    if (!fname)
    {
        puts("missing argument");
        return 1;
    }

    blob = slurp(in);
    if (DBFS_OKAY != dbfs_put(dbfs, (DBFS_FileName){fname}, blob))
    {
        puts("not okay");
        free((uint8_t *)blob.data);
        return 2;
    }
    return 0;
}

static
int do_ovr(DBFS *dbfs, const char *fname, FILE *in)
{
    DBFS_Blob blob;
    if (!fname)
    {
        puts("missing argument");
        return 1;
    }

    blob = slurp(in);
    if (DBFS_OKAY != dbfs_ovr(dbfs, (DBFS_FileName){fname}, blob))
    {
        puts("not okay");
        free((uint8_t *)blob.data);
        return 2;
    }
    return 0;
}

static
int do_del(DBFS *dbfs, const char *fname)
{
    if (!fname)
    {
        puts("missing argument");
        return 1;
    }

    if (DBFS_OKAY != dbfs_del(dbfs, (DBFS_FileName){fname}))
    {
        puts("not okay");
        return 2;
    }
    return 0;
}

static
int do_mvf(DBFS *dbfs, const char *from, const char *to)
{
    if (!from || !to)
    {
        puts("missing argument");
        return 1;
    }
    if (DBFS_OKAY != dbfs_mvf(dbfs, (DBFS_FileName){from}, (DBFS_FileName){to}))
    {
        puts("not okay");
        return 2;
    }
    return 0;
}

int main(int argc, char **argv)
{
    int rv = 0;
    const char *db_name;
    DBFS *dbfs_handle;

    if (argc < 2 || argv[1][0] == '-' || strcmp(argv[1], "help") == 0)
    {
        fprintf(stderr, "Usage: %s <command> [args...]\n", argv[0]);
        fprintf(stderr, "       %s mkd dirname\n", argv[0]);
        fprintf(stderr, "       %s rmd dirname\n", argv[0]);
        fprintf(stderr, "       %s mvd dirname dirname2\n", argv[0]);
        fprintf(stderr, "       %s lsd dirname\n", argv[0]);
        fprintf(stderr, "       %s lsf dirname\n", argv[0]);
        fprintf(stderr, "       %s get filename\n", argv[0]);
        fprintf(stderr, "       %s put filename\n", argv[0]);
        fprintf(stderr, "       %s ovr filename\n", argv[0]);
        fprintf(stderr, "       %s del filename\n", argv[0]);
        fprintf(stderr, "       %s mvf filename filename2\n", argv[0]);
        return 0;
    }

    db_name = getenv("DBFS");
    if (db_name == NULL)
    {
        // in the App, this will be hard-coded
        puts("env DBFS unset, storing in memory (not persistent)");
        db_name = ":memory:";
    }
    dbfs_handle = dbfs_open(db_name);
    if (dbfs_handle == NULL)
    {
        puts("problem opening database");
        return 3;
    }

    if (false)
    {}
    else if (strcmp(argv[1], "mkd") == 0)
    {
        rv = do_mkd(dbfs_handle, argv[2]);
    }
    else if (strcmp(argv[1], "rmd") == 0)
    {
        rv = do_rmd(dbfs_handle, argv[2]);
    }
    else if (strcmp(argv[1], "mvd") == 0)
    {
        rv = do_mvd(dbfs_handle, argv[2], argv[3]);
    }
    else if (strcmp(argv[1], "lsd") == 0)
    {
        rv = do_lsd(dbfs_handle, argv[2], stdout);
    }
    else if (strcmp(argv[1], "lsf") == 0)
    {
        rv = do_lsf(dbfs_handle, argv[2], stdout);
    }
    else if (strcmp(argv[1], "get") == 0)
    {
        rv = do_get(dbfs_handle, argv[2], stdout);
    }
    else if (strcmp(argv[1], "put") == 0)
    {
        rv = do_put(dbfs_handle, argv[2], stdin);
    }
    else if (strcmp(argv[1], "ovr") == 0)
    {
        rv = do_ovr(dbfs_handle, argv[2], stdin);
    }
    else if (strcmp(argv[1], "del") == 0)
    {
        rv = do_del(dbfs_handle, argv[2]);
    }
    else if (strcmp(argv[1], "mvf") == 0)
    {
        rv = do_mvf(dbfs_handle, argv[2], argv[3]);
    }
    else
    {
        printf("unknown command '%s'\n", argv[1]);
        rv = 1;
    }

    dbfs_close(dbfs_handle);
    return rv;
}
