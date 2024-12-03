Get your discord webhook url from Edit Channel > Integrations > Webhooks > New Webhook
Copy the Webhook URL and paste it somewhere because we'll need it after creating a HTTP Post request.

You can test the webhook using curl like this:
curl -X POST -H "Content-Type: application/json" -d '{"content": "This message is sent using curl"}' $WEBHOOK_URL

getUsername Function
--------------------
Using getenv() function with a "USERNAME" parameter gives us the username of the target machine.

C:\Users\test-machine>set USERNAME
USERNAME=test-machine

We could have used the "USERPROFILE" parameter in order to getting the username and the whole path such as 

C:\Users\test-machine>set USERPROFILE
USERPROFILE=C:\Users\test-machine 

but we cannot use it inside the json. Simplest solution, among other solutions, is to get the username and concatenate it with C:\\\\Users\\\\.

findFiles Function
------------------
https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-findfirstfilea
https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-findnextfilea

This function is used for finding the files for given folders (It is hardcoded as Desktop, Documents and Downloads). It uses Windows APIs for file operations, FindFirstFile and FindNextFile.
FindFirstFile and FindNextFile functions are used for searching to find all files in specified folder. And, I would like to point out that asterisk ,'*', is used to find all the files. Without the asterisk, it returns:

Error in FindFirstFile: 2
Error in findFiles in main.

Whichs means that ERROR_FILE_NOT_FOUND. (Search for GetLastError codes in google.)

Another thing to consider is that we need a delimiter for concatinating strings. Using newline ('\n') is not an option because it cannot be used in json (atleast I couldn't make that work). So, I use '#' as a delimiter. Later on, we use this '#' to find all the txt files and read the contents of those.

So the function returns a strings like this: C:\\Users\\test-machine\\Desktop\\file1.txt#C:\\Users\\test-machine\\Documents\\file2.doc#

readContentsofTxtFiles Function
--------------------------------
https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-readfile
https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilea

This function is used for getting the contents of txt files found in findFiles function.
strtok() function is used to split a string by given delimiter. We use this for only taking the txt files. After spliting the 'files', which holds all the files that are seperated by '#' delimiter, into the 'token' varible, We use this token variable to take the files that ends with '.txt' by using strstr function and add it to txtFiles array. If a txt file is found, we open it using CreateFile function and then, we read the contents using ReadFile function and writes contents to content_data variable. After reading the contents, we concatinate txt file name and its contents like this:
C:\\Users\\test-machine\\Desktop\\file1.txt==>Contents of file1.txtC:\\Users\\test-machine\\Documents\\file2.txt==>COntents of file2.txt

sendRequest Function
--------------------
https://learn.microsoft.com/en-us/windows/win32/winhttp/winhttp-sessions-overview

This function is used for sending a POST request to the Discord webhook URL with the given 'data'. It uses the WinHTTP api, Windows library for HTTP operations.
First, we initialize winhttp session using WinHttpOpen function with default configurations. Then, we use WinHttpConnect to establish a connection to "discord.com". After that, we create a POST request with specified endpoint, which is our discord webhook url. After creating a request, it's time send it. Before sending, we create our POST request body part which is a json payload. Our data here is what we want to send as a json payload. After sending, we close all the handles. 

main function
--------------
We concat files and txt files' contents in order to send both. We needed a duplicate of files because we use 'files' variables for readContentsofTxtFiles which changes the data in files variable. After sending the data, we clean up the memory by freeing the allocated memory.
