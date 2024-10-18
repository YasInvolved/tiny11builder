using System;
using System.Collections.Generic;
using System.Linq;
using System.Security.Policy;
using System.Text;
using System.Threading.Tasks;
using System.Diagnostics;
using System.ComponentModel;
using System.Net.Http;
using System.IO;

namespace tiny11builder
{
    public class Constants
    {
        public static readonly string UUPDUMP_URL = "https://api.uupdump.net";
        public static readonly Dictionary<string, string> WINDOWS_VERSIONS = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
        {
            { "23H2", "e93eb708-98ec-494c-b4fc-025c6b012173" },
            { "22H2", "e7036acd-c5d4-46e5-bce0-ccb55d8dbf56" },
            { "21H2", "35630b7b-4509-45b6-83a1-d4c75d5aa9b6" }
        };
    }

    public class Language
    {
        public Language(string langCode, string fancyName)
        {
            LangCode = langCode;
            FancyName = fancyName;
        }

        public override string ToString()
        {
            return FancyName;
        }

        public string LangCode { get; }
        public string FancyName { get; }
    }

    public class Edition
    {
        public Edition(string name, string code)
        {
            Name = name;
            Code = code;
        }

        public override string ToString()
        {
            return Name;
        }

        public string Name { get; }
        public string Code { get; }
    }

    public class UpdateFile
    {
        public UpdateFile(string name, string path, string url, string sha1, int size, string uuid, string debug)
        {
            Name = name;
            Path = path;
            Url = url;
            SHA1 = sha1;
            Size = size;
            UUID = uuid;
            Debug = debug;
        }

        public async Task<bool> Download()
        {
            using (HttpClient client = new HttpClient())
            {
                using (HttpResponseMessage response = await client.GetAsync(Url, HttpCompletionOption.ResponseHeadersRead))
                {
                    response.EnsureSuccessStatusCode();

                    if (!Directory.Exists("updatefiles")) Directory.CreateDirectory("updatefiles");
                    if (File.Exists(Path)) return true;

                    using (Stream contentStream = await response.Content.ReadAsStreamAsync(),
                        fileStream = new FileStream(Path, FileMode.OpenOrCreate, FileAccess.ReadWrite, FileShare.None))
                    {
                        await contentStream.CopyToAsync(fileStream);
                    }
                }
            }

            return true;
        }

        public string Name { get; }
        public string Path {  get; }
        private string Url { get; }
        public string SHA1 { get; }
        public int Size { get; }
        public string UUID { get; }
        public string Debug { get; }
    }

    public class CabinetFile : UpdateFile 
    {
        public CabinetFile(string name, string path, string url, string sha1, int size, string uuid, string debug)
            : base(name, path, url, sha1, size, uuid, debug) { }

        public async Task<bool> Unpack(string destination)
        {
            try
            {
                var processStartInfo = new ProcessStartInfo
                {
                    FileName = "expand.exe",
                    Arguments = $"\"{Path}\" -F:* \"{destination}\"",
                    RedirectStandardError = true,
                    UseShellExecute = false,
                    CreateNoWindow = true,
                };

                if (!Directory.Exists(destination)) Directory.CreateDirectory(destination);

                using (var process = Process.Start(processStartInfo))
                {
                    string stderr = await process.StandardError.ReadToEndAsync();

                    process.WaitForExit();

                    if (string.IsNullOrEmpty(stderr))
                    {
                        Console.WriteLine(stderr);
                    }

                    return process.ExitCode == 0;
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine(ex.ToString());
                return false;
            }
        }
    }

    public class EsdFile : UpdateFile
    {
        public EsdFile(string name, string path, string url, string sha1, int size, string uuid, string debug)
            : base(name, path, url, sha1, size, uuid, debug) { }
    }

    public class Update
    {
        public Update(string updateName, string arch, string build, List<UpdateFile> files)
        {
            UpdateName = updateName;
            Arch = arch;
            Build = build;
            Files = files;
        }

        public string UpdateName { get; }
        public string Arch { get; }
        public string Build { get; }
        public List<UpdateFile> Files { get; }
    }
}
