#include "../threads/malloc.h"
#include "../lib/string.h"

#define MAX_BUF 100

/* helper for parsing filename */
struct token_link
{
  struct token_link *next;
  char *token;
};

/* 
Description : parse filename. args[0] - filename, args[i] - arguments for i > 0.
return : argv (# of arguments)
USE :
   int argv;
   char **args = NULL;

   argv = parse_filename (filename, &args);
*/
int
parse_filename(const char *filename, char ***args)
{
  int argv = 0;
  char *token = NULL, *save_ptr = NULL;
  char buf[MAX_BUF] = {0,};
  struct token_link *token_list = NULL,
                    *temp_token_link = NULL;
  int i = 0;

  strlcpy (buf, filename, sizeof(buf));

  for (token = strtok_r (buf, " ", &save_ptr); token != NULL;
       token = strtok_r (NULL, " ", &save_ptr))
    {
      argv ++;
      
      // insert token into token_list
      if (!token_list)
        {
          token_list = malloc(sizeof(struct token_link));
          token_list->token = token;
          token_list->next = NULL;
        }
      else
        {
          temp_token_link = token_list;
          while (temp_token_link->next)
            {
              temp_token_link = temp_token_link->next;
            }
          
          temp_list->next = malloc(sizeof(struct token_link));
          temp_list->next->token = token;
          temp_list->next->next = NULL;
        }
    }

  *args = malloc(argv * sizeof(char*));

  for (i = 0; i < argv; i++)
    {
      temp_token_link = token_list;
      token_list = token_list->next;
      (*args)[i] = temp_token_link->token;
      free(temp_token_link);
    }

  return argv;
}

/* free arguments in args */
void
free_args(int argv, char ***args)
{
  int i;

  for (i = 0; i < argv; i++)
    {
      free((*args)[i]);
    }
  free(*args);
  *args = NULL;
}
