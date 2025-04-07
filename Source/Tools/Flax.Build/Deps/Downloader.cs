// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.IO;
using System.Net;
using System.Net.Http;
using System.Threading.Tasks;
using Flax.Build;

namespace Flax.Deps
{
    /// <summary>
    /// The helper utility for downloading files from remote url.
    /// </summary>
    static class Downloader
    {
        private const string GoogleDriveDomain = "drive.google.com";
        private const string GoogleDriveDomain2 = "https://drive.google.com";

        // Normal example: FileDownloader.DownloadFileFromURLToPath( "http://example.com/file/download/link", @"C:\file.txt" );
        // Drive example: FileDownloader.DownloadFileFromURLToPath( "http://drive.google.com/file/d/FILEID/view?usp=sharing", @"C:\file.txt" );
        public static FileInfo DownloadFileFromUrlToPath(string url, string path)
        {
            Log.Info(string.Format("Downloading {0} to {1}", url, path));
            if (File.Exists(path))
                File.Delete(path);
            if (url.StartsWith(GoogleDriveDomain) || url.StartsWith(GoogleDriveDomain2))
                return DownloadGoogleDriveFileFromUrlToPath(url, path);
            return DownloadFileFromUrlToPath(url, path, null);
        }

        private static FileInfo DownloadFileFromUrlToPathRaw(string url, string path, HttpClient httpClient)
        {
            if (ProgressDisplay.CanUseConsole)
            {
                using (var progress = new ProgressDisplay(0))
                {
                    var task = DownloadFileFromUrlToPathAsync(url, path, httpClient, progress);
                    task.Wait();
                }
            }
            else
            {
                var task = DownloadFileFromUrlToPathAsync(url, path, httpClient);
                task.Wait();
            }

            return new FileInfo(path);
        }

        private static FileInfo DownloadFileFromUrlToPath(string url, string path, HttpClient httpClient)
        {
            try
            {
                if (httpClient == null)
                {
                    using (httpClient = new HttpClient())
                    {
                        return DownloadFileFromUrlToPathRaw(url, path, httpClient);
                    }
                }
                return DownloadFileFromUrlToPathRaw(url, path, httpClient);
            }
            catch (WebException)
            {
                return null;
            }
        }

        private static async Task DownloadFileFromUrlToPathAsync(string url, string path, HttpClient httpClient, ProgressDisplay progress = null)
        {
            if (progress != null)
            {
                using (var response = await httpClient.GetAsync(url, HttpCompletionOption.ResponseHeadersRead))
                {
                    response.EnsureSuccessStatusCode();

                    var totalBytes = response.Content.Headers.ContentLength;
                    using (var contentStream = await response.Content.ReadAsStreamAsync())
                    {
                        var totalBytesRead = 0L;
                        var readCount = 0L;
                        var buffer = new byte[8192];
                        var hasMoreToRead = true;
                        using (var fileStream = new FileStream(path, FileMode.Create, FileAccess.Write, FileShare.None))
                        {
                            do
                            {
                                var bytesRead = await contentStream.ReadAsync(buffer, 0, buffer.Length);
                                if (bytesRead == 0)
                                {
                                    hasMoreToRead = false;
                                    if (totalBytes.HasValue)
                                        progress.Update(totalBytesRead, totalBytes.Value);
                                    continue;
                                }

                                await fileStream.WriteAsync(buffer, 0, bytesRead);

                                totalBytesRead += bytesRead;
                                readCount += 1;

                                if (readCount % 10 == 0)
                                {
                                    if (totalBytes.HasValue)
                                        progress.Update(totalBytesRead, totalBytes.Value);
                                }
                            } while (hasMoreToRead);
                        }
                    }
                }
            }
            else
            {
                using (var httpStream = await httpClient.GetStreamAsync(url))
                {
                    using (var fileStream = new FileStream(path, FileMode.Create, FileAccess.Write, FileShare.None))
                    {
                        await httpStream.CopyToAsync(fileStream);
                    }
                }
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
            using (var httpClient = new HttpClient())
            {
                FileInfo downloadedFile;

                // Sometimes Drive returns an NID cookie instead of a download_warning cookie at first attempt,
                // but works in the second attempt
                for (int i = 0; i < 2; i++)
                {
                    downloadedFile = DownloadFileFromUrlToPath(url, path, httpClient);
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

                downloadedFile = DownloadFileFromUrlToPath(url, path, httpClient);
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
}
