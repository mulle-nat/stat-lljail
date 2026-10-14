/*
 * statjail.c — minimal reproducer: stat(2) penetrates the lljail jail.
 *
 * Under a Landlock allow-list that does not include the target path, stat()
 * still returns full metadata, while open()/opendir() are denied with EACCES.
 *
 *   usage: statjail [path]
 *          (default path: $HOME, falling back to /home)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>


static void
do_stat( const char *path, int *is_dir)
{
   struct stat   st;

   if( stat( path, &st) == 0)
   {
      printf( "stat(%-28s) = OK    %s size=%lld mode=%o\n",
              path,
              S_ISDIR( st.st_mode) ? "dir " : "file",
              (long long) st.st_size,
              st.st_mode & 07777);
      *is_dir = S_ISDIR( st.st_mode);
   }
   else
   {
      printf( "stat(%-28s) = FAIL  errno=%d (%s)\n",
              path, errno, strerror( errno));
      *is_dir = -1;
   }
}


static void
do_open( const char *path)
{
   int   fd;

   fd = open( path, O_RDONLY);
   if( fd >= 0)
   {
      printf( "open(%-28s) = OK    fd=%d\n", path, fd);
      close( fd);
   }
   else
      printf( "open(%-28s) = FAIL  errno=%d (%s)\n",
              path, errno, strerror( errno));
}


static void
do_readdir( const char *path)
{
   DIR   *dir;

   dir = opendir( path);
   if( dir)
   {
      printf( "opendir(%-25s) = OK\n", path);
      closedir( dir);
   }
   else
      printf( "opendir(%-25s) = FAIL  errno=%d (%s)\n",
              path, errno, strerror( errno));
}


/*
 * If the target is a directory, find a regular file inside it. stat() finds
 * it even though the directory itself cannot be listed; its contents are
 * then denied, which is the same hole on a file instead of a directory.
 */
static void
do_file_in( const char *dir)
{
   static const char *names[] = {
      ".profile", ".bashrc", ".bash_history", ".gitconfig", NULL
   };
   const char    *name;
   char          path[ 4096];
   struct stat   st;
   int           i;
   int           is_dir;

   for( i = 0; names[ i]; i++)
   {
      snprintf( path, sizeof( path), "%s/%s", dir, names[ i]);
      if( stat( path, &st) == 0)
      {
         do_stat( path, &is_dir);
         do_open( path);
         return;
      }
   }
}


int
main( int argc, char *argv[])
{
   const char   *target;
   int          is_dir;

   target = (argc > 1) ? argv[ 1] : getenv( "HOME");
   if( ! target || ! *target)
      target = "/home";

   printf( "forbidden path: %s\n\n", target);

   printf( "-- outside the allow-list (contents must be denied) --\n");
   do_stat( target, &is_dir);
   if( is_dir == 1)
   {
      do_readdir( target);
      do_file_in( target);
   }
   else
      if( is_dir == 0)
         do_open( target);

   printf( "\n-- control: inside the allow-list (/tmp) --\n");
   do_stat( "/tmp", &is_dir);
   if( is_dir == 1)
      do_readdir( "/tmp");

   return( 0);
}
