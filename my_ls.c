// fiskov@gmail.com     2022-09-11
// ls-utility, only "ls -l"

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

#define FILE_COUNT_CHUNK    100
#define NAME_LENGTH_MAX     256
#define TIME_LENGTH_MAX     64

//common types
typedef struct file_info_t
{    
    int type;
    int attrib;
    char *name;
    char *linkname;
    unsigned user_id, group_id;
    unsigned link_cnt;
    size_t size;
    time_t mtime;
} file_info_t;

typedef struct counters_t
{
    unsigned files, total, sectors;
} counters_t;

typedef struct cols_info_t 
{
    unsigned size, name, user, group, links;
} cols_info_t;


//static vars
static counters_t counters = {.total = FILE_COUNT_CHUNK};
static file_info_t *p_file_info = NULL;
static cols_info_t cols = {};


// get files in directory
static int fill_file_info(const char *dir)
{
    struct dirent *pDirent;
    struct stat stat_tmp;
    
    DIR *pDir = opendir (dir);
    if (pDir == NULL)
        return 1;
    chdir(dir);

    int cnt = 0;
    while ((pDirent = readdir(pDir)) != NULL) 
    {
        // increase memory
        if (counters.files >= counters.total )
        {
            counters.total += FILE_COUNT_CHUNK;            
            p_file_info = realloc(p_file_info, counters.total * sizeof(file_info_t));
        }

        lstat(pDirent->d_name, &stat_tmp);

        //hidden file (flag `-a`) isn't realised
        if (pDirent->d_name[0] != '.')
        {
            file_info_t *p_file = &p_file_info[counters.files++];
            
            p_file->name = malloc(strlen(pDirent->d_name));
            strcpy(p_file->name, pDirent->d_name);
            
            p_file->type = pDirent->d_type;

            p_file->user_id  = stat_tmp.st_uid;
            p_file->group_id = stat_tmp.st_gid;
            p_file->size     = stat_tmp.st_size;
            p_file->attrib   = stat_tmp.st_mode;
            p_file->mtime    = stat_tmp.st_mtime;
            p_file->link_cnt = stat_tmp.st_nlink;

            // get name for link
            if (S_ISLNK(stat_tmp.st_mode))
            {
                char *bfr = malloc(NAME_LENGTH_MAX);
                int size = readlink(pDirent->d_name, bfr, NAME_LENGTH_MAX);
                bfr[size]='\0';
                
                p_file->linkname = malloc(size+1);
                strcpy(p_file->linkname, bfr);
                
                free(bfr);
            }
        }
    }

    chdir("..");
    return 0;
}


// get user-id in /etc/passwd
static void get_user_name(char *bfr, uid_t uid)
{
    //TODO make lookup table for users

    struct passwd *pwd;
    if ((pwd = getpwuid(uid)) != NULL)
        strcpy(bfr, pwd->pw_name);
    else 
        bfr[0] = '\0';  // user-name not fount
}

static void get_group_name(char *bfr, gid_t gid)
{
    //TODO make lookup table for groups

   struct group *grp;
    if ((grp = getgrgid(gid)) != NULL)
        strcpy(bfr, grp->gr_name);
    else 
        bfr[0] = '\0';  // user-name not fount
}

static int get_digits_count(size_t number)
{
  int result = 0;
  
  while (number != 0) {
    number /= 10;
    result++;
  }
 
  return result;
}

// get  maximum col-width for files
static void calculate_column_width(void)
{
    file_info_t *p_file;
    unsigned length;
    char *bfr = malloc(NAME_LENGTH_MAX);

    for (int i=0; i < counters.files; i++)
    { 
        p_file = (file_info_t *)&p_file_info[i];

        get_user_name(bfr, p_file->user_id);
        length = strlen(bfr) + 1;
        if ( length > cols.user )
            cols.user = length;

        get_group_name(bfr, p_file->group_id);
        length = strlen(bfr) + 1;
        if ( length > cols.group )
            cols.group = length;

        length = get_digits_count(p_file->size);
        if ( length > cols.size )
            cols.size = length;

        length = get_digits_count(p_file->link_cnt);
        if ( length > cols.links )
            cols.links = length;            
    }

    free(bfr);
}


static char get_type_char(int type)
{                 // 0123456789012345
    char c_type[] = "?pc-d-b---l-s-w";   // look dirent.h enum DT_*
    return c_type[type];
}
static void make_symbol_attrib(char *bfr, unsigned attrib)
{
    const char attrib_string[] = "rwxrwxrwx";
    for (int i=0; i<9; i++)
        bfr[i] = ((attrib >> (8-i)) & 0x1 ? attrib_string[i] : '-');
    bfr[9] = '\0';
}

static void convert_time_to_string(char *bfr, time_t mtime, time_t now)
{
    struct tm tm_file, tm_now;

    localtime_r(&mtime, &tm_file);
    localtime_r(&now, &tm_now);

    // if year is curent, print time
    if (tm_file.tm_year == tm_now.tm_year)
        strftime (bfr, TIME_LENGTH_MAX, "%b %e %H:%M", &tm_file);    
    else
        strftime (bfr, TIME_LENGTH_MAX, "%b %e  %Y",  &tm_file);    
}

static void print_file_info(void)
{
    file_info_t *p_file;
    time_t now = time (NULL);

    char *bfr = malloc(NAME_LENGTH_MAX);
    char *str = malloc(NAME_LENGTH_MAX);

    char c_type = '-';

    for (int i=0; i<counters.files; i++)
    {
        int pos = 0;

        p_file = (file_info_t *)&p_file_info[i];

        //type + attrib
        c_type = get_type_char(p_file->type);
        make_symbol_attrib(bfr, p_file->attrib);        
        pos += sprintf(str, "%c%s %*d", c_type, bfr, cols.links, p_file->link_cnt);

        // username
        get_user_name(bfr, p_file->user_id);
        pos += sprintf(&str[pos], "%*s", cols.user, bfr);

        // group
        get_group_name(bfr, p_file->group_id);
        pos += sprintf(&str[pos], "%*s ", cols.group, bfr);

        // size
        pos += sprintf(&str[pos], "%*ld ", cols.size, p_file->size);

        // datetime
        convert_time_to_string(bfr, p_file->mtime, now);
        pos += sprintf(&str[pos], "%s ", bfr);

        // filename
        pos += sprintf(&str[pos], "%s", p_file->name);

        // link
        if (p_file->linkname != NULL)
            pos += sprintf(&str[pos], " -> %s", p_file->linkname);

        printf("%s\n", str);
    }

    free(str);
    free(bfr);
}



static int compare_names(const void* a, const void* b)
{    
    return strcasecmp(((file_info_t*)a)->name, ((file_info_t*)b)->name);
}
static void sort_files(void)
{
    if (counters.files > 0)
        qsort(p_file_info, counters.files, sizeof(file_info_t), compare_names );
}



int main(int argc, char *argv[])
{
    char dir[NAME_LENGTH_MAX] = ".";

    if (argc < 2 || argc > 3 || strcmp(argv[1], "-l"))
    {
        printf("Usage: my_ls -l\n");
        exit(1);
    }
    // if user specified a dir
    if (argc == 3)
        strcpy(dir, argv[2]);

    p_file_info = malloc(sizeof(file_info_t) * counters.total);

    int res = fill_file_info(dir);

    if (res)    
        printf("Directory not found\n");
    else 
    {
        calculate_column_width();

        sort_files();
        
        print_file_info();
    }

    // free memory
    for (int i=0; i<counters.files; i++)
    {
        file_info_t *p_file = &p_file_info[i];
        free(p_file->name);
        if (p_file->linkname != NULL)
            free(p_file->linkname);
    }

    free(p_file_info);

    return 0;
}
