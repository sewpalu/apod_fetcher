#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "curl/curl.h"
#include "gumbo.h"

#define APOD_URI "https://apod.nasa.gov/apod/astropix.html"
#define IMAGE_BASE_ADDRESS "https://apod.nasa.gov/apod/"

#define SET_CURL_OPTION(opt, val)                                              \
  res = curl_easy_setopt(curl_handle, opt, val);                               \
  if (res != CURLE_OK)                                                         \
    printf("Unable to set curl option: %d", res);
#define CALL_CURL_PERFORM()                                                    \
  res = curl_easy_perform(curl_handle);                                        \
  if (res != CURLE_OK)                                                         \
    printf("Unable to perform request: %d", res);

char* fetch_site(char* uri);
void fetch_site_to_file(char* uri, char* filename);
char* extract_picture_uri(char* site);

int main(int argc, char** argv)
{
  if (argc != 2)
  {
    printf("usage: %s <DEST>\n", argv[0]);
    return EXIT_FAILURE;
  }

  char* site = NULL;
  char* picture_uri = NULL;

  printf("Requesting %s\n", APOD_URI);
  site = fetch_site(APOD_URI);
  printf("Received %lu bytes\n", site ? strlen(site) : 404);

  picture_uri = extract_picture_uri(site);
  printf("Picture URI: %s\n", picture_uri ? picture_uri : "");

  printf("Requesting %s\n", picture_uri ? picture_uri : "");
  fetch_site_to_file(picture_uri, argv[1]);

  if (site)
    free(site);
  if (picture_uri)
    free(picture_uri);

  return EXIT_SUCCESS;
}

static size_t on_receive_body(void* in, size_t always_one, size_t in_len,
                              char** out)
{
  (void)always_one;

  size_t old_len = *out ? strlen(*out) : 0;
  size_t new_len = old_len + in_len;

  *out = realloc(*out, new_len + 1);
  if (!*out)
    return 0;

  memcpy((*out) + old_len, in, in_len);
  (*out)[new_len] = '\0';

  return in_len;
}

// Returned value is dynamically allocated and must be freed
char* fetch_site(char* uri)
{
  CURL* curl_handle = curl_easy_init();
  if (!curl_handle)
    return NULL;

  CURLcode res;
  SET_CURL_OPTION(CURLOPT_URL, uri);

  char* body = NULL;
  SET_CURL_OPTION(CURLOPT_WRITEDATA, &body);
  SET_CURL_OPTION(CURLOPT_WRITEFUNCTION, on_receive_body);
  SET_CURL_OPTION(CURLOPT_FAILONERROR, 1);

  CALL_CURL_PERFORM();

  curl_easy_cleanup(curl_handle);

  return body;
}

void fetch_site_to_file(char* uri, char* output_filename)
{
  FILE* outfile = fopen(output_filename, "w");
  if (!outfile)
    return;

  CURL* curl_handle = curl_easy_init();
  if (!curl_handle)
    return;

  CURLcode res;

  SET_CURL_OPTION(CURLOPT_URL, uri);
  SET_CURL_OPTION(CURLOPT_WRITEDATA, outfile);
  SET_CURL_OPTION(CURLOPT_FAILONERROR, 1);

  CALL_CURL_PERFORM();

  curl_easy_cleanup(curl_handle);

  if (outfile)
    fclose(outfile);
}

static GumboNode* get_image_node(GumboNode* head)
{
  if (!head || head->type != GUMBO_NODE_ELEMENT)
    return NULL;

  if (head->v.element.tag == GUMBO_TAG_IMG)
    return head;

  for (size_t i = 0u; i < head->v.element.children.length; ++i)
  {
    GumboNode* res = get_image_node(head->v.element.children.data[i]);
    if (res)
      return res;
  }

  return NULL;
}

// Returned value is dynamically allocated and must be freed
char* extract_picture_uri(char* site)
{
  GumboOutput* gumbo_output = gumbo_parse(site);
  GumboNode* img_node = get_image_node(gumbo_output->root);

  char* ret = NULL;

  if (img_node)
  {
    assert(img_node->type == GUMBO_NODE_ELEMENT);
    GumboAttribute* img_link =
        gumbo_get_attribute(&img_node->v.element.attributes, "src");

    if (img_link)
    {
      size_t len = strlen(IMAGE_BASE_ADDRESS) + strlen(img_link->value);
      ret = malloc(len + 1);
      strcpy(ret, IMAGE_BASE_ADDRESS);
      strcat(ret, img_link->value);
    }
  }

  gumbo_destroy_output(&kGumboDefaultOptions, gumbo_output);
  return ret;
}

