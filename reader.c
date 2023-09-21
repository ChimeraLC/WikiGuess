// This file handles the getting of the wikipedia page alongside parsing the words
 
#include <stdio.h>
#include <stdint.h>
#include <curl/curl.h>
#include "bloom.h"

struct page_data {
	char *data;
	uint64_t len;
};

// Callback function used for 
uint64_t 
write_callback(void *new_data, uint64_t size, uint64_t nmemb, struct page_data *p_data)
{
	// Calculate new size required
	uint64_t new_len = p_data->len + size*nmemb;
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
parse_page(char *wiki_page)  {

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
	// Estimate required number of values
	uint64_t estimate = p_data.len/5; 	// Average of 5 letters per word
	init_filter(estimate, 0.001);
	u_int64 total_words = 0;
	// Parse through contents
	int bracket_count = 0;	// Skip delimiters
	while (right_pointer[0] != '\0') {
		// As long as we are within a delimiter, skip values
		if (right_pointer[0] == '<') {
			if (bracket_count == 0 && left_pointer < right_pointer - 1) {
				right_pointer[0] = '\0';
				//printf("%s ", left_pointer);
				insert(left_pointer);
				total_words++;
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
					// Insert into filter
					//printf("%s ", left_pointer);
					insert(left_pointer);
					total_words++;
				}
				right_pointer++;				// TODO: does not accurately cover all cases, like \u characters
				left_pointer = right_pointer + 1;
			}
			// Find word endings
			if (right_pointer[0] == ',' || right_pointer[0] == ' ' || // Includes most common word separaters
			right_pointer[0] == '(' || right_pointer[0] == ')' || 
			right_pointer[0] == '.' || right_pointer[0] == ':') {
				right_pointer[0] = '\0';			// Grab the most recently passed word
				if (left_pointer < right_pointer - 1) {
					// Insert into filter
					//printf("%s ", left_pointer);
					insert(left_pointer);
					total_words++;
				}
				left_pointer = right_pointer + 1;
			}
		}
		// Continue to next character
		right_pointer++;
	}
	// Display total words
	printf("Total words: %lld\n", total_words);

	//printf("%s\n", p_data.data);

	// Check false positive rate
	/*
	int fp = 0;
	int length = 10;
	for (int i = 0; i < 10000; i++) {
		// Set of characters
		static char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789,.-#'?!";      
		// Malloc out length  
		char *random_string = malloc(sizeof(char) * (length +1));
     
		// Generate random keys
		for (int j = 0; j < length; j++) {            
			int key = rand() % (int)(sizeof(charset) -1);
			random_string[j] = charset[key];
		}
		// Set end char
		random_string[length] = '\0';

		// Test bloom filter
		if (test(random_string)) {
			fp++;
		}

		free(random_string);
	}
	printf("FP rate: %f\n", (float)fp/10000);
	
	// Check if properly contained
        char data[100];
	while (true) {
                printf("Enter data: ");
                scanf("%s", data);
                if (strcmp(data, "quit") == 0) {
                        break;
                }
		if (test(data)) {
			printf("Contains\n");
		}
		else {
			printf("Not contained\n");
		}
	}
	*/

	// Free data before exiting
	free(p_data.data);
	return 0;

}