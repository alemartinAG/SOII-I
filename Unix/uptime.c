#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
int
find_nth_space(char *search_buffer,
               int   space_ordinality
              )
{
  int jndex;
  int space_count;

  space_count=0;

  for(jndex=0;
      search_buffer[jndex];
      jndex++
     )
  {
    if(search_buffer[jndex]==' ')
    {
      space_count++;

      if(space_count>=space_ordinality)
      {
        return jndex;
      }
    }
  }

  fprintf(stderr,"looking for too many spaces\n");

  exit(1);

} /* find_nth_space() */

int
main(int    argc,
     char **argv
    )
{
  int       field_begin;
  int       stat_fd;

  char      proc_buf[80];
  char      stat_buf[2048];

  long      jiffies_per_second;

  long long boot_time_since_epoch;
  long long process_start_time_since_boot;

  time_t    process_start_time_since_epoch;

  ssize_t   read_result;

  struct tm gm_buf;
  struct tm local_buf;

  jiffies_per_second=sysconf(_SC_CLK_TCK);

  if(argc<2)
  {
    strcpy(proc_buf,"/proc/self/stat");
  }
  else
  {
    sprintf(proc_buf,"/proc/%ld/stat",strtol(argv[1],NULL,0));
  }

  for(;;)
  {
    stat_fd=open(proc_buf,O_RDONLY);

    if(stat_fd<0)
    {
      fprintf(stderr,"open() fail\n");

      exit(1);
    }

    read_result=read(stat_fd,stat_buf,sizeof(stat_buf));

    if(read_result<0)
    {
      fprintf(stderr,"read() fail\n");

      exit(1);
    }

    if(read_result>=sizeof(stat_buf))
    {
      fprintf(stderr,"stat_buf is too small\n");

      exit(1);
    }

    field_begin=find_nth_space(stat_buf,21)+1;

    stat_buf[find_nth_space(stat_buf,22)]=0;

    sscanf(stat_buf+field_begin,"%llu",&process_start_time_since_boot);

    close(stat_fd);

    stat_fd=open("/proc/stat",O_RDONLY);

    if(stat_fd<0)
    {
      fprintf(stderr,"open() fail\n");

      exit(1);
    }

    read_result=read(stat_fd,stat_buf,sizeof(stat_buf));

    if(read_result<0)
    {
      fprintf(stderr,"read() fail\n");

      exit(1);
    }

    if(read_result>=sizeof(stat_buf))
    {
      fprintf(stderr,"stat_buf is too small\n");

      exit(1);
    }

    close(stat_fd);

    field_begin=strstr(stat_buf,"btime ")-stat_buf+6;

    sscanf(stat_buf+field_begin,"%llu",&boot_time_since_epoch);

    process_start_time_since_epoch
    =
    boot_time_since_epoch+process_start_time_since_boot/jiffies_per_second;

    localtime_r(&process_start_time_since_epoch,&local_buf);
    gmtime_r   (&process_start_time_since_epoch,&gm_buf   );

    printf("local time: %02d:%02d:%02d\n",
           local_buf.tm_hour,
           local_buf.tm_min,
           local_buf.tm_sec
          );

    printf("UTC:        %02d:%02d:%02d\n",
           gm_buf.tm_hour,
           gm_buf.tm_min,
           gm_buf.tm_sec
          );

    sleep(1);
  }

  return 0;
} /* main() */