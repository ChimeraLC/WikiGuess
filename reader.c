// This file handles the getting of the wikipedia page alongside parsing the words
 
#include <stdio.h>
#include <curl/curl.h>
#include "bloom.h"

struct page_data {
	char *data;
	size_t len;
};

// Callback function used for 
size_t 
write_callback(void *new_data, size_t size, size_t nmemb, struct page_data *p_data)
{
	// Calculate new size required
	size_t new_len = p_data->len + size*nmemb;
	// Allocate new space for data
	char *ptr = realloc(p_data->data, new_len+1);
	// Check that realloc succeeds
	if (ptr == NULL) {
		fprintf(stderr, "Realloc failed\n");
		exit(EXIT_FAILURE);
	}
	p_data->data = ptr;
	memcpy(p_data->data+p_data->len, new_data, size*nmemb);
	p_data->data[new_len] = '\0';
	p_data->len = new_len;

	return size*nmemb;
}

int
main(int argc, char **argv)  {

	// Parse the input arguments
	if (argc < 2) {
		printf("usage: %s Wikipedia Page Title\n", argv[0]);
		return 1;
	}
	// Combine remaining arguments as wiki title				// Todo, parsing special characters?
	char *wiki_page = argv[1];
	for (int i = 2; i < argc; i++) {
		strcat(wiki_page, " ");
		strcat(wiki_page, argv[i]);
	}

	// Replace spaces with '_'
	char *cur_pos = strchr(wiki_page, ' ');
	while (cur_pos) {
	*cur_pos = '_';
	cur_pos = strchr(cur_pos + 1, ' ');
	}

	printf("Seeking page subject: %s\n", wiki_page);

	// Create corresponding wikipedia mediawiki api url
	char url[200]; 	// Length of page name should not exceed 100
	sprintf(url, "%s%s%s", 
		"https://en.wikipedia.org/w/api.php?action=query&format=json&titles=",
		wiki_page,
		"&prop=extracts"
	);

	// Use libcurl to get website content
	CURL *curl;
	CURLcode res;

	// Create string to store website data
	struct page_data p_data;

	// Create initial values
	p_data.len = 0;
	p_data.data = malloc(1);	// Initial malloc
	if (p_data.data == NULL) {	// Malloc failed
		fprintf(stderr, "Malloc failed\n");
		return 1;
	}
	p_data.data[0] = '\0';	// Place end of string

	// Initialize curl
	curl_global_init(CURL_GLOBAL_DEFAULT);
	curl = curl_easy_init();
	if(curl) {	// If curl initialization succeeds
		curl_easy_setopt(curl, CURLOPT_URL, url);
		// Write output to string s
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &p_data);

		// Perform the request, res will get the return code
		res = curl_easy_perform(curl);
		// Check for errors
		if(res != CURLE_OK)
		fprintf(stderr, "curl_easy_perform() failed: %s\n",
			curl_easy_strerror(res));

		// always cleanup
		curl_easy_cleanup(curl);
	}
	// Return error if curl initialization failed
	else {
		fprintf(stderr, "Failed to initialize curl");
		return 1;
	}
	// Global cleanup
	curl_global_cleanup();

	// Parse through the html data
	char *left_pointer = p_data.data;
	char *right_pointer = p_data.data;

	// Locate pages info
	left_pointer = strstr(p_data.data, "pages") + 9;
	right_pointer = strstr(left_pointer, "\"");

	right_pointer[0] = '\0';

	int page_count = atoi(left_pointer);					// TODO: perhaps better way to check
	if (page_count == -1) {
		printf("Page not found.");
		return 1;
	}

	// Locate page title
	left_pointer = strstr(right_pointer + 1, "title") + 8;
	right_pointer = strstr(left_pointer, "\"");
	right_pointer[0] = '\0';
	
	char *title = left_pointer;
	printf("Page title: %s\n", title);

	// Locate page extract
	left_pointer = strstr(right_pointer + 1, "extract") + 10;
	right_pointer = left_pointer;

	// Initialize bloom filter
	init_filter(100, 0.01);

	// Parse through contents
	int bracket_count = 0;	// Skip delimiters
	while (right_pointer[0] != '\0') {
		if (right_pointer[0] == '<') {
			if (left_pointer < right_pointer - 1) {
				right_pointer[0] = '\0';
				printf("%s ", left_pointer);
			}
			bracket_count++;
		}
		if (right_pointer[0] == '>') {
			bracket_count--;
			if (bracket_count == 0) {
				left_pointer = right_pointer + 1;
			}
		}

		if (bracket_count == 0) {
			// Skip escape characters
			if (right_pointer[0] == '\\') {
				right_pointer[0] = '\0';
				if (left_pointer < right_pointer - 1) {
					printf("%s ", left_pointer);
				}
				right_pointer++;				// TODO: does not accurately cover all cases
				left_pointer = right_pointer + 1;
			}
			// Find word endings
			if (right_pointer[0] == ',' || right_pointer[0] == ' ' || 
			right_pointer[0] == '(' || right_pointer[0] == ')' || 
			right_pointer[0] == '.' || right_pointer[0] == ':') {
				right_pointer[0] = '\0';
				if (left_pointer < right_pointer - 1) {
					printf("%s ", left_pointer);
					//insert(left_pointer);
				}
				left_pointer = right_pointer + 1;
			}
		}
		right_pointer++;
	}

	//printf("%s\n", p_data.data);
	free(p_data.data);

	printf("Completed");
	return 0;

}