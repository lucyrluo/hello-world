#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

// from https://curl.haxx.se/libcurl/c/CURLOPT_HTTPHEADER.html
//  gcc -o curlHttpHeader curlHttpHeader.c -lcurl
int main()
{
    curl_global_init(CURL_GLOBAL_ALL);
    CURL *curl = curl_easy_init();
    struct curl_slist *list = NULL;
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "http://example.com");

        list = curl_slist_append(list, "Shoesize: 10");
        list = curl_slist_append(list, "User-Agent: SonicWall");
        list = curl_slist_append(list, "Accept: */*");

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);

        curl_easy_perform(curl);

        CURLcode res;
        char *ct;
        long val;
        /* ask for the content-type */
        res = curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &ct);
        if ((CURLE_OK == res) && ct)
            printf("\nWe received Content-Type: %s\n", ct);

        res = curl_easy_getinfo(curl, CURLINFO_HTTP_CONNECTCODE, &val);
        if (CURLE_OK == res)
            printf("\nWe received Http-Connect: %d\n", val);

        res = curl_easy_getinfo(curl, CURLINFO_NUM_CONNECTS, &val);
        if (CURLE_OK == res)
            printf("\nWe received Number-Connect: %d\n", val);

        curl_slist_free_all(list); /* free the list again */
    }
}