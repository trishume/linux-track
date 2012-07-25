#include "rest.h"
#define _GNU_SOURCE
#include <stdio.h>
#include <windows.h>
#include <commctrl.h>

bool game_data_get_desc(int id, game_desc_t *gd)
{
  FILE *f = NULL;
  HKEY hkey = 0;
  int res = 0;
  RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\Linuxtrack", 0, 
    KEY_QUERY_VALUE, &hkey);
  if(!hkey){
    printf("Can't open registry key\n");
    return false;
  }
  
  BYTE buf[1024];
  DWORD buf_len = sizeof(buf)-1; //To be sure there is a space for null
  LONG result = RegQueryValueEx(hkey, "Data", NULL, NULL, buf, &buf_len);
  if((result == ERROR_SUCCESS) && (buf_len > 0)){
    int size = sizeof(WCHAR) * (lstrlen(buf) + 1);
    WCHAR *path = malloc(size);
    MultiByteToWideChar(CP_UNIXCP, 0, buf, -1, path, size);
    if((f = fopen(wine_get_unix_file_name(path), "r"))== NULL){
      printf("Can't open data file '%s'!\n", wine_get_unix_file_name(path));
      return false;
    }
    int tmp_id;
    size_t tmp_str_size = 4096; 
    size_t tmp_code_size = 4096;
    char *tmp_str = malloc(tmp_str_size);
    char *tmp_code = malloc(tmp_code_size);
    unsigned int c1, c2;
    int cnt;
    gd->name = NULL;
    while(!feof(f)){
      cnt = getline(&tmp_str, &tmp_str_size, f);
      if(cnt > 0){
        if(tmp_str_size > tmp_code_size){
          tmp_code = realloc(tmp_code, tmp_str_size);
        }
        cnt = sscanf(tmp_str, "%d \"%[^\"]\" (%08x%08x)", &tmp_id, tmp_code, &c1, &c2);
        if(cnt == 2){
          if(tmp_id == id){
            gd->name = strdup(tmp_code);
            gd->encrypted = false;
            gd->key1 = gd->key2 = 0;
            break;
          }
        }else if(cnt == 4){
          if(tmp_id == id){
            gd->name = strdup(tmp_code);
            gd->encrypted = true;
            gd->key1 = c1;
            gd->key2 = c2;
            break;
          }
        }
      }
    }
    fclose(f);
    free(tmp_code);
    free(tmp_str);
  }
  RegCloseKey(hkey);
  return gd->name != NULL;
}


