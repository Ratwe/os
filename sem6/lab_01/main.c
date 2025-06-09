#include "apue.h"

#include <dirent.h>
#include <limits.h>
#include <errno.h>

#include <locale.h>
#include <time.h>

typedef int Myfunc(const char *);

static Myfunc myfunc;
static Myfunc myfunc_empty;
static int myftw(char *, Myfunc *);
static int dopath(Myfunc *);
static int myftw_chdir(char *, Myfunc *);
static int dopath_chdir(Myfunc *, char *);

#ifdef PATH_MAX
static long pathmax = PATH_MAX;
#else
static long pathmax = 0;
#endif

static long posix_version = 0;
static long xsi_version = 0;

#define PATH_MAX_GUESS 1024

const double TIMEDIVISION = 1e6;

static char* fullpath;
static size_t pathlen;

static char* fullpath_chdir;
static size_t pathlen_chdir;

double alloc_time;
double alloc_time_chdir;


static int depth = 0;

// просто выделяет память размером PATH_MAX или PATH_MAX_GUESS
char* path_alloc(size_t *sizep) /* если удалось выделить память, возвращает также выделенной объем */
{
    char *ptr;
    size_t size;

    if (posix_version == 0)
        posix_version = sysconf(_SC_VERSION);
    if (xsi_version == 0)
        xsi_version = sysconf(_SC_XOPEN_VERSION);

    if (pathmax == 0)
    { /* первый вызов функции */
        errno = 0;
        if ((pathmax = pathconf("/", _PC_PATH_MAX)) < 0)
        {
            if (errno == 0)
                pathmax = PATH_MAX_GUESS; /* если константа не определена */
            else
                err_sys("ошибка вызова pathconf с параметром _PC_PATH_MAX");
        }
        else
            pathmax++; /* добавить 1, так как путь относительно корня */
    }

    /*
    * До версии POSIX.1-2001 не гарантируется, что PATH_MAX включает
    * завершающий нулевой байт. То же для XPG3.
    */
    if ((posix_version < 200112L) && (xsi_version < 4))
        size = pathmax + 1;
    else
        size = pathmax;

    ptr = malloc(size);
    
    if (ptr == NULL)
        err_sys("ошибка вызова malloc");
    if (sizep != NULL)
        *sizep = size;
        
    return(ptr);
}

long getCpuTime(void)
{
	struct timespec t;
	if (clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t))
		return -1;
	return t.tv_sec * 1000000000LL + t.tv_nsec;
}

int main(int argc, char * argv[])
{
    int ret, ret_chdir, ret_time, ret_chdir_time;
    long s;

    if (argc != 2)
        err_quit("Необходимо указать имя стартового каталога\n");

    ret = myftw(argv[1], myfunc);

    printf("\n\n");
    depth = 0;

    ret_chdir = myftw_chdir(argv[1], myfunc);

    depth = 0;

    double res1 = 0;
    for (size_t i = 0; i < 10; ++i)
    {
        // printf("i = %ld\n", i);
        s = getCpuTime();
        ret_time = myftw(argv[1], myfunc_empty);
        res1 += (getCpuTime() - s) / TIMEDIVISION;
        //printf("res1 for %ld = %lf\n", i, res1);
    }
    res1 /= 10;

    depth = 0;

    double res2 = 0;
    for (size_t i = 0; i < 10; ++i)
    {
        s = getCpuTime();
        ret_chdir_time = myftw_chdir(argv[1], myfunc_empty);
        res2 += (getCpuTime() - s) / TIMEDIVISION;
        //printf("res2 for %ld = %lf\n", i, res2);
    }
    res2 /= 10;


    printf("\n\n\nРеализация без chdir: %lf мс\n"
            "Реализация с chdir: %lf мс\n\n\n", res1, res2);

    printf("Время, затраченное на выделение памяти: %lf мс\n", alloc_time /= 1000);
    
    exit(ret + ret_chdir);
}

/*
 * Выполняет обход дерева каталогов, начиная с каталога "pathname".
 * Для каждого встреченного файла вызывает пользовательскую функцию func().
 */

/* возвращает то, что вернула функция func() */
static int myftw(char *pathname, Myfunc *func)
{
    long s = getCpuTime();
    fullpath = path_alloc(&pathlen); /* выделить память для PATH_MAX+1 байт */
    alloc_time += (getCpuTime() - s) / TIMEDIVISION;
    
    if (pathlen <= strlen(pathname))
    {
        pathlen = strlen(pathname) * 2;
        fullpath = realloc(fullpath, pathlen);
        if (fullpath == NULL)
            err_sys("ошибка вызова realloc");
    }
    strcpy(fullpath, pathname);

    //printf("return dopath()\n");    
    return dopath(func);
}

static int myftw_chdir(char *pathname, Myfunc *func)
{
    fullpath_chdir = path_alloc(&pathlen_chdir); /* выделить память для PATH_MAX+1 байт */
    
    if (pathlen_chdir <= strlen(pathname))
    {
        pathlen_chdir = strlen(pathname) * 2;
        fullpath_chdir = realloc(fullpath_chdir, pathlen_chdir);
        if (fullpath_chdir == NULL)
            err_sys("ошибка вызова realloc");
    }
    strcpy(fullpath_chdir, pathname);

    return dopath_chdir(func, pathname);
}

/*
 * Выполняет обход дерева каталогов, начиная с "fullpath".
 * Если "fullpath" не является каталогом, для него вызывается lstat(),
 * func() и затем выполняется возврат.
 * Для каталогов производится рекурсивный вызов функции.
 */

/* возвращает то, что вернула функция func() */
static int dopath(Myfunc *func)
{
    struct stat statbuf;
    struct dirent *dirp;
    DIR *dp;
    int ret, n;

    if (lstat(fullpath, &statbuf) < 0) /* ошибка вызова функции stat */
        return(func(fullpath));
    if (S_ISDIR(statbuf.st_mode) == 0) /* не каталог */
        return(func(fullpath));
    /*
    * Это каталог. Сначала вызвать функцию func(),
    * а затем обработать все файлы в этом каталоге.
    */
    if ((ret = func(fullpath)) != 0)
        return(ret);

    n = strlen(fullpath);
    if (n + NAME_MAX + 2 > pathlen)
    { /* увеличить размер буфера */
        pathlen *= 2;
        fullpath = realloc(fullpath, pathlen);
        if (fullpath == NULL)
            err_sys("ошибка вызова realloc");
    }
    fullpath[n++] = '/';
    fullpath[n] = 0;

    if ((dp = opendir(fullpath)) == NULL) /* каталог недоступен */
        return(func(fullpath));

    depth++;
    while ((dirp = readdir(dp)) != NULL)
    {
        //printf("dopath() depth = %d\n", depth);
        // пропуск скрытых файлов
        if (dirp->d_name[0] == '.')
            continue;

        if (strcmp(dirp->d_name, ".") == 0 || strcmp(dirp->d_name, "..") == 0)
            continue; /* пропустить каталоги "." и ".." */

        strcpy(&fullpath[n], dirp->d_name); /* добавить имя после слеша */
        if ((ret = dopath(func)) != 0) /* рекурсия */
            break; /* выход по ошибке */
    }
    depth--;

    fullpath[n-1] = 0; /* стереть часть строки от слеша и до конца */

    if (closedir(dp) < 0)
        err_ret("невозможно закрыть каталог %s", fullpath);

    return ret;
}

static int dopath_chdir(Myfunc *func, char *path)
{
    struct stat statbuf;
    struct dirent *dirp;
    DIR *dp;
    int ret, n;

    if (lstat(path, &statbuf) < 0) /* ошибка вызова функции stat */
        return func(path);
    if (S_ISDIR(statbuf.st_mode) == 0) /* не каталог */
        return func(path);

    /*
     * Это каталог. Сначала вызвать функцию func(),
     * а затем обработать все файлы в этом каталоге.
     */
    if ((ret = func(path)) != 0)
        return ret;

    if ((dp = opendir(path)) == NULL) /* каталог недоступен */
        return func(path);

    chdir(path);
    depth++;
    while ((dirp = readdir(dp)) != NULL)
    {
        // пропуск скрытых файлов
        if (dirp->d_name[0] == '.')
            continue;

        if (strcmp(dirp->d_name, ".") == 0 || strcmp(dirp->d_name, "..") == 0 || dirp->d_name[0] == '.')
            continue; /* пропустить каталоги "." и ".." */
        
        if ((ret = dopath_chdir(func, dirp->d_name)) != 0) /* рекурсия */
            break; /* выход по ошибке */

        //printf("depth = %d\n", depth);
    }
    depth--;
    if (depth > 0)
        chdir("..");

    //printf("chdir .. completed\n");

    if (closedir(dp) < 0)
        err_ret("невозможно закрыть каталог %s", path);

    //printf("trying return ret\n");

    return ret;
}

int myfunc(const char *pathname)
{
    const char* last_slash = strrchr(pathname, '/');
    char* erased_path = (last_slash == NULL) ? (char*)pathname : (char*)(last_slash + 1);

    if (depth == 0)
        printf("%s\n", pathname);
    else {
        for (int i = 0; i < depth - 1; i++)
            printf("│   ");
        printf("└── ");
        printf("%s\n", erased_path);
    }

    return 0;
}

int myfunc_empty(const char *pathname)
{
    return 0;
}