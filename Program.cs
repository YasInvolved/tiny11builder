using System;
using System.Linq;
using System.Collections.Generic;
using System.Threading.Tasks;
using System.Net.Http;
using Newtonsoft.Json.Linq;
using System.Threading;
using System.Runtime.InteropServices;

namespace tiny11builder
{
    public static class Utils
    {
        public static async Task<List<Language>> GetLanguages(string versionId)
        {
            List<Language> languages = new List<Language>();

            using (HttpClient client = new HttpClient())
            {
                HttpResponseMessage response = await client.GetAsync($"{Constants.UUPDUMP_URL}/listlangs.php?id={versionId}");
                response.EnsureSuccessStatusCode();
                string responseBody = await response.Content.ReadAsStringAsync();
                JObject jsonObject = JObject.Parse(responseBody);
                JObject langFancyNames = (JObject)jsonObject["response"]["langFancyNames"];

                foreach (var entry in langFancyNames)
                {
                   languages.Add(new Language(entry.Key, entry.Value.ToString()));
                }
            }

            return languages;
        }

        public static async Task<List<Edition>> GetEditions(string versionId)
        {
            List<Edition> editions = new List<Edition>();

            using (HttpClient client = new HttpClient())
            {
                HttpResponseMessage response = await client.GetAsync($"{Constants.UUPDUMP_URL}/listeditions.php?id={versionId}");
                response.EnsureSuccessStatusCode();
                string responseBody = await response.Content.ReadAsStringAsync();
                JObject jsonObject = JObject.Parse(responseBody);
                JObject editionFancyNames = (JObject)jsonObject["response"]["editionFancyNames"];

                foreach (var entry in editionFancyNames)
                {
                    editions.Add(new Edition(entry.Value.ToString(), entry.Key));
                }
            }

            return editions;
        }

        public static async Task<Update> GetUpdate(string versionId, Language lang, Edition edition)
        {
            using (HttpClient client = new HttpClient())
            {
                using (HttpResponseMessage response = await client.GetAsync($"{Constants.UUPDUMP_URL}/get.php?id={versionId}&lang={lang.LangCode}&edition={edition.Code}"))
                {
                    response.EnsureSuccessStatusCode();
                    JObject body = (JObject)JObject.Parse(await response.Content.ReadAsStringAsync())["response"];

                    string updateName = (string)body["updateName"];
                    string arch = (string)body["arch"];
                    string build = (string)body["build"];

                    List<UpdateFile> files = new List<UpdateFile>();

                    foreach (var file in body["files"].Children<JProperty>())
                    {
                        if (file.Name.EndsWith(".cab"))
                        {
                            files.Add(new CabinetFile(
                                file.Name, 
                                $"updatefiles/{file.Name}",
                                (string)file.Value["url"],
                                (string)file.Value["sha1"], 
                                (int)file.Value["size"], 
                                (string)file.Value["uuid"], 
                                (string)file.Value["debug"]
                            ));
                        }
                    }

                    return new Update(updateName, arch, build, files);
                }
            }
        }

        public static T GetInputForDictionary<T>(string text, Dictionary<string, T> options)
        {
            while (true)
            {
                Console.WriteLine($"{text}");
                foreach (var pair in options)
                {
                    Console.WriteLine($"- {pair.Key}");
                }

                Console.Write("\n\nPlease choose an option: ");
                string input = Console.ReadLine().ToUpper();

                if (options.ContainsKey(input)) return options[input];
                Console.WriteLine("Wrong value inserted. Please retry.\n\n\n\n");
                Thread.Sleep(2000);
            }
        }

        public static T GetInputForArray<T>(string text, List<T> options)
        {
            while (true)
            {
                Console.WriteLine($"{text}");
                for (int i = 0; i < options.Count; i++)
                {
                    Console.WriteLine($"{i}: {options[i]}");
                }

                Console.Write("\n\nPlease choose an index: ");
                string input = Console.ReadLine();

                if (int.TryParse(input, out int value) && value < options.Count) return options[value];
                Console.WriteLine("Wrong value inserted. Please retry.\n\n\n\n");
                Thread.Sleep(2000);
            }
        }
    }
    internal class Program
    {
        static void Main(string[] args)
        {
            Console.Title = "Tiny11Builder";
            Console.WriteLine("Welcome to tiny11builder!\n\n\n\n");

            string versionId = Utils.GetInputForDictionary("Select Windows Version: ", Constants.WINDOWS_VERSIONS);

            var languages = Utils.GetLanguages(versionId);
            var editions = Utils.GetEditions(versionId);

            languages.Wait();
            Language language = Utils.GetInputForArray("Select the language", languages.Result);
            Console.WriteLine($"Selected: {language}");
            
            editions.Wait();
            Edition edition = Utils.GetInputForArray("Select desired edition", editions.Result);
            Console.WriteLine($"Selected: {edition}");

            var update = Utils.GetUpdate(versionId, language, edition);
            update.Wait();

            Console.WriteLine($"Fetched update.\n{update.Result.UpdateName}\nArch: {update.Result.Arch}\nBuild ID: {update.Result.Build}");

            List<Task<bool>> tasks = new List<Task<bool>>();
            foreach (var file in update.Result.Files)
            {
                tasks.Add(file.Download());
                if (file is CabinetFile && file.Path.ToLower().Contains("aggregatedmetadata"))
                {
                    tasks.Add(((CabinetFile)file).Unpack("metadata"));
                }
            }
            
            Task.WaitAll(tasks.ToArray());
            Console.WriteLine("Downloading completed");

            Console.ReadKey();
            Console.Beep();
        }
    }
}
