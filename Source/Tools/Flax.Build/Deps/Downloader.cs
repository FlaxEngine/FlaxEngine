// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Net;
using System.Threading;
using Flax.Build;

namespace Flax.Deps
{
    /// <summary>
    /// The helper utility for downloading files from remote url.
    /// </summary>
    static class Downloader
    {
        /*
        private static void DownloadFile(string fileUrl, string fileTmp)
        {
            HttpWebRequest webRequest = WebRequest.CreateHttp(fileUrl);
            webRequest.Method = "GET";
            webRequest.Timeout = 3000;
            using (WebResponse webResponse = webRequest.GetResponse())
            {
                using (Stream remoteStream = webResponse.GetResponseStream())
                {
                    DownloadFile(remoteStream, webResponse.ContentLength, fileTmp);
                }
            }
        }

        private static void DownloadFile(Stream remoteStream, long length, string fileTmp)
        {
            long bytesProcessed = 0;
            byte[] buffer = new byte[1024];

            using (var localStream = new FileStream(fileTmp, FileMode.Create, FileAccess.Write, FileShare.Read))
            {
                using (var display = new ProgressDisplay(length))
                {
                    int bytesRead;
                    do
                    {
                        bytesRead = remoteStream.Read(buffer, 0, buffer.Length);
                        localStream.Write(buffer, 0, bytesRead);
                        bytesProcessed += bytesRead;
                        display.Progress = bytesProcessed;
                    } while (bytesRead > 0);
                }
            }
        }
        */

        private const string GoogleDriveDomain = "drive.google.com";
        private const string GoogleDriveDomain2 = "https://drive.google.com";

        // Normal example: FileDownloader.DownloadFileFromURLToPath( "http://example.com/file/download/link", @"C:\file.txt" );
        // Drive example: FileDownloader.DownloadFileFromURLToPath( "http://drive.google.com/file/d/FILEID/view?usp=sharing", @"C:\file.txt" );

        public static FileInfo DownloadFileFromUrlToPath(string url, string path)
        {
            Log.Verbose(string.Format("Downloading {0} to {1}", url, path));

            if (url.StartsWith(GoogleDriveDomain) || url.StartsWith(GoogleDriveDomain2))
                return DownloadGoogleDriveFileFromUrlToPath(url, path);
            return DownloadFileFromUrlToPath(url, path, null);
        }

        private static FileInfo DownloadFileFromUrlToPathRaw(string url, string path, WebClient webClient)
        {
            if (ProgressDisplay.CanUseConsole)
            {
                using (var display = new ProgressDisplay(0))
                {
                    DownloadProgressChangedEventHandler downloadProgress = (sender, e) => { display.Update(e.BytesReceived, e.TotalBytesToReceive); };
                    AsyncCompletedEventHandler downloadCompleted = (sender, e) =>
                    {
                        lock (e.UserState)
                        {
                            Monitor.Pulse(e.UserState);
                        }
                    };

                    var syncObj = new object();
                    lock (syncObj)
                    {
                        webClient.DownloadProgressChanged += downloadProgress;
                        webClient.DownloadFileCompleted += downloadCompleted;

                        webClient.DownloadFileAsync(new Uri(url), path, syncObj);
                        Monitor.Wait(syncObj);

                        webClient.DownloadProgressChanged -= downloadProgress;
                        webClient.DownloadFileCompleted -= downloadCompleted;
                    }
                }
            }
            else
            {
                AsyncCompletedEventHandler downloadCompleted = (sender, e) =>
                {
                    lock (e.UserState)
                    {
                        if (e.Error != null)
                        {
                            Log.Error("Download failed.");
                            Log.Exception(e.Error);
                        }

                        Monitor.Pulse(e.UserState);
                    }
                };

                var syncObj = new object();
                lock (syncObj)
                {
                    webClient.DownloadFileCompleted += downloadCompleted;

                    webClient.DownloadFileAsync(new Uri(url), path, syncObj);
                    Monitor.Wait(syncObj);

                    webClient.DownloadFileCompleted -= downloadCompleted;
                }
            }

            return new FileInfo(path);
        }

        private static FileInfo DownloadFileFromUrlToPath(string url, string path, WebClient webClient)
        {
            try
            {
                if (webClient == null)
                {
                    using (webClient = new WebClient())
                    {
                        return DownloadFileFromUrlToPathRaw(url, path, webClient);
                    }
                }

                return DownloadFileFromUrlToPathRaw(url, path, webClient);
            }
            catch (WebException)
            {
                return null;
            }
        }

        // Downloading large files from Google Drive prompts a warning screen and
        // requires manual confirmation. Consider that case and try to confirm the download automatically
        // if warning prompt occurs
        private static FileInfo DownloadGoogleDriveFileFromUrlToPath(string url, string path)
        {
            // You can comment the statement below if the provided url is guaranteed to be in the following format:
            // https://drive.google.com/uc?id=FILEID&export=download
            url = GetGoogleDriveDownloadLinkFromUrl(url);

            using (CookieAwareWebClient webClient = new CookieAwareWebClient())
            {
                FileInfo downloadedFile;

                // Sometimes Drive returns an NID cookie instead of a download_warning cookie at first attempt,
                // but works in the second attempt
                for (int i = 0; i < 2; i++)
                {
                    downloadedFile = DownloadFileFromUrlToPath(url, path, webClient);
                    if (downloadedFile == null)
                        return null;

                    // Confirmation page is around 50KB, shouldn't be larger than 60KB
                    if (downloadedFile.Length > 60000)
                        return downloadedFile;

                    // Downloaded file might be the confirmation page, check it
                    string content;
                    using (var reader = downloadedFile.OpenText())
                    {
                        // Confirmation page starts with <!DOCTYPE html>, which can be preceeded by a newline
                        char[] header = new char[20];
                        int readCount = reader.ReadBlock(header, 0, 20);
                        if (readCount < 20 || !(new string(header).Contains("<!DOCTYPE html>")))
                            return downloadedFile;

                        content = reader.ReadToEnd();
                    }

                    int linkIndex = content.LastIndexOf("href=\"/uc?", StringComparison.Ordinal);
                    if (linkIndex < 0)
                        return downloadedFile;

                    linkIndex += 6;
                    int linkEnd = content.IndexOf('"', linkIndex);
                    if (linkEnd < 0)
                        return downloadedFile;

                    url = "https://drive.google.com" + content.Substring(linkIndex, linkEnd - linkIndex).Replace("&amp;", "&");
                }

                downloadedFile = DownloadFileFromUrlToPath(url, path, webClient);

                return downloadedFile;
            }
        }

        // Handles 3 kinds of links (they can be preceeded by https://):
        // - drive.google.com/open?id=FILEID
        // - drive.google.com/file/d/FILEID/view?usp=sharing
        // - drive.google.com/uc?id=FILEID&export=download
        public static string GetGoogleDriveDownloadLinkFromUrl(string url)
        {
            int index = url.IndexOf("id=", StringComparison.Ordinal);
            int closingIndex;
            if (index > 0)
            {
                index += 3;
                closingIndex = url.IndexOf('&', index);
                if (closingIndex < 0)
                    closingIndex = url.Length;
            }
            else
            {
                index = url.IndexOf("file/d/", StringComparison.Ordinal);
                if (index < 0) // url is not in any of the supported forms
                    return string.Empty;

                index += 7;

                closingIndex = url.IndexOf('/', index);
                if (closingIndex < 0)
                {
                    closingIndex = url.IndexOf('?', index);
                    if (closingIndex < 0)
                        closingIndex = url.Length;
                }
            }

            return string.Format("https://drive.google.com/uc?id={0}&export=download", url.Substring(index, closingIndex - index));
        }
    }

    /// <summary>
    /// A <see cref="WebClient"/> with exposed Response property.
    /// </summary>
    /// <seealso cref="System.Net.WebClient" />
    public class WebClientWithResponse : WebClient
    {
        /// <summary>
        /// Gets the response.
        /// </summary>
        public byte[] Response { get; private set; }

        /// <inheritdoc />
        protected override WebResponse GetWebResponse(WebRequest request)
        {
            var response = base.GetWebResponse(request);

            if (response is HttpWebResponse httpResponse)
            {
                using (var stream = httpResponse.GetResponseStream())
                {
                    using (var ms = new MemoryStream())
                    {
                        stream.CopyTo(ms);
                        Response = ms.ToArray();
                    }
                }
            }

            return response;
        }
    }

    /// <summary>
    /// Web client used for Google Drive
    /// </summary>
    /// <seealso cref="System.Net.WebClient" />
    public class CookieAwareWebClient : WebClient
    {
        private class CookieContainer
        {
            private readonly Dictionary<string, string> _cookies;

            public string this[Uri url]
            {
                get
                {
                    string cookie;
                    if (_cookies.TryGetValue(url.Host, out cookie))
                        return cookie;

                    return null;
                }
                set { _cookies[url.Host] = value; }
            }

            public CookieContainer()
            {
                _cookies = new Dictionary<string, string>();
            }
        }

        private readonly CookieContainer _cookies;

        /// <summary>
        /// Initializes a new instance of the <see cref="CookieAwareWebClient"/> class.
        /// </summary>
        public CookieAwareWebClient()
        {
            _cookies = new CookieContainer();
        }

        /// <inheritdoc />
        protected override WebRequest GetWebRequest(Uri address)
        {
            WebRequest request = base.GetWebRequest(address);

            if (request is HttpWebRequest)
            {
                string cookie = _cookies[address];
                if (cookie != null)
                    ((HttpWebRequest)request).Headers.Set("cookie", cookie);
            }

            return request;
        }

        /// <inheritdoc />
        protected override WebResponse GetWebResponse(WebRequest request, IAsyncResult result)
        {
            WebResponse response = base.GetWebResponse(request, result);

            string[] cookies = response.Headers.GetValues("Set-Cookie");
            if (cookies != null && cookies.Length > 0)
            {
                string cookie = "";
                foreach (string c in cookies)
                    cookie += c;

                _cookies[response.ResponseUri] = cookie;
            }

            return response;
        }

        /// <inheritdoc />
        protected override WebResponse GetWebResponse(WebRequest request)
        {
            WebResponse response = base.GetWebResponse(request);

            string[] cookies = response.Headers.GetValues("Set-Cookie");
            if (cookies != null && cookies.Length > 0)
            {
                string cookie = "";
                foreach (string c in cookies)
                    cookie += c;

                _cookies[response.ResponseUri] = cookie;
            }

            return response;
        }
    }
}
