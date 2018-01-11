#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define SEPERATE_BODY   1

#define GOOGLE_URL "https://finance.google.com/finance?"

#define YAHOO_URL  "http://finance.yahoo.com/d/quotes.csv?"

enum
{
    OUTPUT_HTML,
    OUTPUT_JSON,
    //last one
    MAX_OUTPUT_FORMAT
};
enum
{
    GOOGLE_DATA,
    YAHOO_DATA, 
    //last one
    MAX_DATA_SOURCE
};
enum
{
    BUILDIN_SYM,
    READ_PROFILE,
};
char *patterns[MAX_DATA_SOURCE][MAX_OUTPUT_FORMAT] =
{
    { "span id=\"ref_",  "\"l\" : " },
    { "unknown", "unknown" }
};
#define LAST_SYM    "XXXX"
char *buildInSym[] = {
    "AMZN", "GOOG", "AAPL", "INTC",
    LAST_SYM
};
int searchInFile(const char *fname, int source, int format)
{
    char tmpString[1024];
    char *patten = patterns[source][format];
    if (source == YAHOO_DATA)
        return;     // does not work yet.
    if (format == OUTPUT_HTML) {
        sprintf(tmpString, "cat %s | grep -m1 -E '%s' | awk -F \">\" '{print $2}' | awk -F \"<\" '{print $1}'", fname, patten);
        system(tmpString);
    }
    else if (format == OUTPUT_JSON) {
        sprintf(tmpString, "cat %s | grep -m1 -E '%s'", fname, patten);
        system(tmpString);
    }
}
// the string in first MAX_CHAR of a line
void searchInFileLimited(const char *fname, int source, int format, char *sym)
{
#define MAX_CHAR 256
    int fd, r, j = 0;
    char temp, line[MAX_CHAR+1];
    char *patten = patterns[source][format];

    if ((fd = open(fname, O_RDONLY)) != -1)
    {
        while ((r = read(fd, &temp, sizeof(char))) != 0)
        {
            if (temp != '\n')
            {
                if (j < MAX_CHAR)
                {
                    line[j] = temp;
                    j++;
                }
            }
            else
            {
                line[j] = '\0';
                if (strstr(line, patten) != NULL)
                {
                    printf("found : %s\n", line);
                    char * pchS, *pchE;
                    pchS = strchr(line, '>'); // ex  <span id="ref_304466804484872_l">111<
                    if (pchS != NULL)
                    {
                        pchE = strchr(pchS, '<');
                        if (pchE != NULL)
                        {
                            *pchE = '\0';
                            printf("%s:%s\n", sym, pchS+1);                            
                        }
                    }
                    break;
                }
                memset(line, 0, sizeof(line));
                j = 0;
            }
        }
    }
    else
    {
        printf("%s open file %s failed\n", __FUNCTION__, fname);
    }
    close(fd);
}
static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
    //printf("%s, handle %p, size %d nmemb %d\n", __FUNCTION__, stream, size, nmemb);
    size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
    return written;
}
int getOneProfile(CURL *curl, int source, int format, char *sym);
int getProfile(CURL *curl, int source, int format, char *fname)
{
    FILE *profile = NULL;
    profile = fopen(fname, "r");
    if (profile == NULL)
    {
        printf("failed to open profile(%s)\n", fname);
    }
    char tmpBuf[1024];
    while (fgets(tmpBuf, sizeof(tmpBuf), profile))
    {
        tmpBuf[strlen(tmpBuf)-1] = '\0';    //remove '\n'
        //printf("%s\n", tmpBuf);
        getOneProfile(curl, source, format, tmpBuf);
    }
    fclose(profile);
}
//  gcc -o curlGetSSSProfile curlGetSSSProfile.c -lcurl
int main(int argc, char **argv)
{
    curl_global_init(CURL_GLOBAL_ALL);
    CURL *curl = curl_easy_init();

    int output_format = OUTPUT_HTML;
    int data_source = GOOGLE_DATA;
    int mode = BUILDIN_SYM;
#define MAX_RPOFILE_NAME 64
    char profile[MAX_RPOFILE_NAME];
    if (argc == 2)
    {
        if (strcasecmp(argv[1], "json") == 0)
            output_format = OUTPUT_JSON;
        else if (strcasecmp(argv[1], "html") == 0)
            output_format = OUTPUT_HTML;
        else {
            mode = READ_PROFILE;
            if (strlen(argv[1]) > MAX_RPOFILE_NAME)
            {
                printf("profile name %s is too long. MAX %d\n", argv[1], MAX_RPOFILE_NAME);
                return -1;
            }
            strcpy(profile, argv[1]);
        }
    }
    if (argc == 3)
    {
        if (strcasecmp(argv[1], "json") == 0)
            output_format = OUTPUT_JSON;
        else if (strcasecmp(argv[1], "html") == 0)
            output_format = OUTPUT_HTML;

        if (strcasecmp(argv[2], "yahoo") == 0)
            data_source = YAHOO_DATA;
        else if (strcasecmp(argv[2], "google") == 0)
            data_source = GOOGLE_DATA;
    }
    if (mode == READ_PROFILE)
    {
        getProfile(curl, data_source, output_format, profile);
    }
    else if (mode == BUILDIN_SYM)
    {
        int i = 0;
        while (1)
        {
            if (strcmp(buildInSym[i], LAST_SYM) == 0)
                break;
            getOneProfile(curl, data_source, output_format, buildInSym[i]);
            i++;
        }
    }
    // clean up before exit.
    curl_global_cleanup();
}
int getOneProfile(CURL *curl, int source, int format, char *sym)
{
    static const char *bodyfilename = "body.out";
    FILE *bodyfile = NULL;
    char tmpString[1024];
    CURLcode res;
    if(source == GOOGLE_DATA)
        sprintf(tmpString, "%s%s&q=%s", GOOGLE_URL, (format == OUTPUT_HTML)?"output=html":"output=json", sym);
    else if (source == YAHOO_DATA)
        sprintf(tmpString, "%s%s&s=%s", YAHOO_URL, (format == OUTPUT_HTML)?"output=html":"output=json", sym);

    printf("cmd:%s\n", tmpString);

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, tmpString);

#ifdef SKIP_PEER_VERIFICATION
        /*
        * If you want to connect to a site who isn't using a certificate that is
        * signed by one of the certs in the CA bundle you have, you can skip the
        * verification of the server's certificate. This makes the connection
        * A LOT LESS SECURE.
        *
        * If you have a CA cert for the server stored someplace else than in the
        * default bundle, then the CURLOPT_CAPATH option might come handy for
        * you.
        */
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
#endif
#if SEPERATE_BODY
        /* send all data to this function  */
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        /* open the body file */
        bodyfile = fopen(bodyfilename, "wb");
        if (!bodyfile) {
            curl_easy_cleanup(curl);
            return -1;
        }
        /* we want the body be written to this file handle instead of stdout */
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, bodyfile);
#endif
        curl_easy_perform(curl);

        if (CURLE_OK == res) {
        }

        /* close the body file */
        if(bodyfile)
            fclose(bodyfile);

        if (bodyfile)
        {
            //searchInFile(bodyfilename, source, format);
            searchInFileLimited(bodyfilename, source, format, sym);
        }
    }
}