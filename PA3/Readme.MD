Author: Vipraja Patil

Description:
A simple web proxy server that is capable of relaying HTTP requests from clients to the HTTP servers. The webproxy parses the request from the client and checks if the request method is 'GET' otherwise throws '400 Bad request'. If the url requested for the first time, then after fetching the data, it is stored in cache. So, the next time same url is requested the data is fetched from the cache rather that sending the request again. The cache is cleared after a certain time (cache timeout is given by the user as an argument while running the program). For caching and searching file, MD5 hashing is been used.

To run the program,
./webproxy <port no> <cache timeout value>
  
For testing,
Clear the cache or use private window.
For Testing with http browser (Mozilla Firefox)
http://netstech.org
http://neverssl.com


