/*
  File autogenerated by gengetopt version 2.10
  generated with the following command:
  gengetopt -i strreduction.ggo -u -F strreduction_cmdline 

  The developers of gengetopt consider the fixed text that goes in all
  gengetopt output files to be in the public domain:
  we make no copyright claims on it.
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* If we use autoconf.  */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "getopt.h"

#include "strreduction_cmdline.h"

void
cmdline_parser_print_version (void)
{
  printf ("%s %s\n", CMDLINE_PARSER_PACKAGE, CMDLINE_PARSER_VERSION);
}

void
cmdline_parser_print_help (void)
{
  cmdline_parser_print_version ();
  printf("\n"
  "Purpose:\n"
  "  perform Statistical Substring Reduction operation on input N-grams\n"
  "  \n"
  "  example: strreduction -f5 -a2 file\n"
  "  will do SSR operation on input word N-gram strings with merging threshold set\n"
  "  to 5 (-f5) using algorithm 2 (-a2).\n"
  "\n"
  "Usage: %s [OPTIONS]... [FILES]...\n", CMDLINE_PARSER_PACKAGE);
  printf("   -h         --help           Print help and exit\n");
  printf("   -V         --version        Print version and exit\n");
  printf("   -FSTRING   --from=STRING    input stream encoding (for character ngram only) (default='UTF-8')\n");
  printf("   -TSTRING   --to=STRING      output stream encoding (for character ngram only) (default='UTF-8')\n");
  printf("   -oSTRING   --output=STRING  write output to file,use stdout if omitted\n");
  printf("   -aINT      --algorithm=INT  using Nth reduction algorithm (default='2')\n");
  printf("   -c         --char           enter char gram mode, default is word ngram model (default=off)\n");
  printf("   -s         --sort           sort result (default=off)\n");
  printf("   -t         --time           show time cost by reduction algorithm on stderr (not including I/O) (default=off)\n");
  printf("   -fINT      --freq=INT       frequence threshold needed in algorithm 1,2,4 (default='1')\n");
  printf("   -mINT      --m1=INT         minimum n-gram need in algorithm 4 (default='1')\n");
}


static char *gengetopt_strdup (const char *s);

/* gengetopt_strdup() */
/* strdup.c replacement of strdup, which is not standard */
char *
gengetopt_strdup (const char *s)
{
  char *result = (char*)malloc(strlen(s) + 1);
  if (result == (char*)0)
    return (char*)0;
  strcpy(result, s);
  return result;
}

int
cmdline_parser (int argc, char * const *argv, struct gengetopt_args_info *args_info)
{
  int c;	/* Character of the parsed option.  */
  int missing_required_options = 0;

  args_info->help_given = 0 ;
  args_info->version_given = 0 ;
  args_info->from_given = 0 ;
  args_info->to_given = 0 ;
  args_info->output_given = 0 ;
  args_info->algorithm_given = 0 ;
  args_info->char_given = 0 ;
  args_info->sort_given = 0 ;
  args_info->time_given = 0 ;
  args_info->freq_given = 0 ;
  args_info->m1_given = 0 ;
#define clear_args() { \
  args_info->from_arg = gengetopt_strdup("UTF-8") ;\
  args_info->to_arg = gengetopt_strdup("UTF-8") ;\
  args_info->output_arg = NULL; \
  args_info->algorithm_arg = 2 ;\
  args_info->char_flag = 0;\
  args_info->sort_flag = 0;\
  args_info->time_flag = 0;\
  args_info->freq_arg = 1 ;\
  args_info->m1_arg = 1 ;\
}

  clear_args();

  args_info->inputs = NULL;
  args_info->inputs_num = 0;

  optarg = 0;
  optind = 1;
  opterr = 1;
  optopt = '?';

  while (1)
    {
      int option_index = 0;
      char *stop_char;

      static struct option long_options[] = {
        { "help",	0, NULL, 'h' },
        { "version",	0, NULL, 'V' },
        { "from",	1, NULL, 'F' },
        { "to",	1, NULL, 'T' },
        { "output",	1, NULL, 'o' },
        { "algorithm",	1, NULL, 'a' },
        { "char",	0, NULL, 'c' },
        { "sort",	0, NULL, 's' },
        { "time",	0, NULL, 't' },
        { "freq",	1, NULL, 'f' },
        { "m1",	1, NULL, 'm' },
        { NULL,	0, NULL, 0 }
      };

      stop_char = 0;
      c = getopt_long (argc, argv, "hVF:T:o:a:cstf:m:", long_options, &option_index);

      if (c == -1) break;	/* Exit from `while (1)' loop.  */

      switch (c)
        {
        case 'h':	/* Print help and exit.  */
          clear_args ();
          cmdline_parser_print_help ();
          exit (EXIT_SUCCESS);

        case 'V':	/* Print version and exit.  */
          clear_args ();
          cmdline_parser_print_version ();
          exit (EXIT_SUCCESS);

        case 'F':	/* input stream encoding (for character ngram only).  */
          if (args_info->from_given)
            {
              fprintf (stderr, "%s: `--from' (`-F') option given more than once\n", CMDLINE_PARSER_PACKAGE);
              clear_args ();
              exit (EXIT_FAILURE);
            }
          args_info->from_given = 1;
          args_info->from_arg = gengetopt_strdup (optarg);
          break;

        case 'T':	/* output stream encoding (for character ngram only).  */
          if (args_info->to_given)
            {
              fprintf (stderr, "%s: `--to' (`-T') option given more than once\n", CMDLINE_PARSER_PACKAGE);
              clear_args ();
              exit (EXIT_FAILURE);
            }
          args_info->to_given = 1;
          args_info->to_arg = gengetopt_strdup (optarg);
          break;

        case 'o':	/* write output to file,use stdout if omitted.  */
          if (args_info->output_given)
            {
              fprintf (stderr, "%s: `--output' (`-o') option given more than once\n", CMDLINE_PARSER_PACKAGE);
              clear_args ();
              exit (EXIT_FAILURE);
            }
          args_info->output_given = 1;
          args_info->output_arg = gengetopt_strdup (optarg);
          break;

        case 'a':	/* using Nth reduction algorithm.  */
          if (args_info->algorithm_given)
            {
              fprintf (stderr, "%s: `--algorithm' (`-a') option given more than once\n", CMDLINE_PARSER_PACKAGE);
              clear_args ();
              exit (EXIT_FAILURE);
            }
          args_info->algorithm_given = 1;
          args_info->algorithm_arg = strtol (optarg,&stop_char,0);
          break;

        case 'c':	/* enter char gram mode, default is word ngram model.  */
          if (args_info->char_given)
            {
              fprintf (stderr, "%s: `--char' (`-c') option given more than once\n", CMDLINE_PARSER_PACKAGE);
              clear_args ();
              exit (EXIT_FAILURE);
            }
          args_info->char_given = 1;
          args_info->char_flag = !(args_info->char_flag);
          break;

        case 's':	/* sort result.  */
          if (args_info->sort_given)
            {
              fprintf (stderr, "%s: `--sort' (`-s') option given more than once\n", CMDLINE_PARSER_PACKAGE);
              clear_args ();
              exit (EXIT_FAILURE);
            }
          args_info->sort_given = 1;
          args_info->sort_flag = !(args_info->sort_flag);
          break;

        case 't':	/* show time cost by reduction algorithm on stderr (not including I/O).  */
          if (args_info->time_given)
            {
              fprintf (stderr, "%s: `--time' (`-t') option given more than once\n", CMDLINE_PARSER_PACKAGE);
              clear_args ();
              exit (EXIT_FAILURE);
            }
          args_info->time_given = 1;
          args_info->time_flag = !(args_info->time_flag);
          break;

        case 'f':	/* frequence threshold needed in algorithm 1,2,4.  */
          if (args_info->freq_given)
            {
              fprintf (stderr, "%s: `--freq' (`-f') option given more than once\n", CMDLINE_PARSER_PACKAGE);
              clear_args ();
              exit (EXIT_FAILURE);
            }
          args_info->freq_given = 1;
          args_info->freq_arg = strtol (optarg,&stop_char,0);
          break;

        case 'm':	/* minimum n-gram need in algorithm 4.  */
          if (args_info->m1_given)
            {
              fprintf (stderr, "%s: `--m1' (`-m') option given more than once\n", CMDLINE_PARSER_PACKAGE);
              clear_args ();
              exit (EXIT_FAILURE);
            }
          args_info->m1_given = 1;
          args_info->m1_arg = strtol (optarg,&stop_char,0);
          break;


        case 0:	/* Long option with no short option */

        case '?':	/* Invalid option.  */
          /* `getopt_long' already printed an error message.  */
          exit (EXIT_FAILURE);

        default:	/* bug: option not considered.  */
          fprintf (stderr, "%s: option unknown: %c\n", CMDLINE_PARSER_PACKAGE, c);
          abort ();
        } /* switch */
    } /* while */


  if ( missing_required_options )
    exit (EXIT_FAILURE);

  if (optind < argc)
    {
      int i = 0 ;
  
      args_info->inputs_num = argc - optind ;
      args_info->inputs = 
        (char **)(malloc ((args_info->inputs_num)*sizeof(char *))) ;
      while (optind < argc)
        args_info->inputs[ i++ ] = gengetopt_strdup (argv[optind++]) ; 
    }
  
  return 0;
}