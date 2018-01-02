"""
Python script to produce a localized version of the whole repository.

It works by replacing all patterns in the format "____[a-zA-Z_0-9]____" with
a translation stored in a csv table (column headers are the different locales
like "en", "de", "es" and so).

It consists of three parts:

- extract
  - extracts all strings with the pattern mentioned above
  - puts them into a csv file
  - updates this file as necessary
  - gets a directory as an argument and extracts strings from all files
- stats
  - provides simple statistics on how many strings are translated for each language
- rewriter
  - gets a source directory, a target directory and a locale
  - replaces all pattern strings with their localized version during copying
- gcc_wrapper
  - acts like a wrapper for gcc, that can be used instead of gcc
  - uses the rewriter to copy the files to a temporary location and
    then adjusts their paths to let gcc run on the translated versions


Notes
-----
- gzip .minimized (in the rewriter)
- rewriter also does the encoding

Technical side nodes
--------------------
- no dependencies (besides python)
- should be platform independent (except the gcc wrapper)
- should be documented
- the tool config file lays in the same directory that this file lays in

Todo
----
- test it
- write gcc wrapper or scrap the proposal
- start internationalizing the POV code

"""
import argparse
import csv
import gzip
import json
import os
import re
import traceback
from _warnings import warn
from collections import OrderedDict
from pathlib import Path

import sys
from typing import Optional, Dict, List, Union, Set, Callable, IO

import shutil


class TranslateException(BaseException):
    """
    Base exception for all exceptions related to the logic of this tool
    """

    def __init__(self, msg: str):
        super(TranslateException, self).__init__(msg)


class Environment:
    """
    Captures the configuration and contains small platform independent helper methods.
    """

    CONFIG_FILE = "translate.config"
    ID_PATTERN_MIDDLE = "[0-9_A-Za-z]+"
    ID_REGEXP_MIDDLE = re.compile(ID_PATTERN_MIDDLE)
    DEFAULT_CONFIG = {
        "pattern_prefix": "___", # start of a translation id pattern
        "pattern_suffix": "___", # end of a translation id pattern
        "translation_file": "translations.csv",  # relative path to the translations file
        "translated_file_endings": [".h", ".hpp", ".cpp", ".c", ".json", ".ino", ".html"], # endings of files that are translated
        "target_encoding": "utf-8",
        "files_to_gzip_pattern": ".min.", # if a file name contains this string, then the file will be zipped in post processing
        "gzipped_file_ending": ".gz"
    }

    def __init__(self, base_path: Path, config: dict):
        self.base_path = base_path
        self.config = {}
        if isinstance(config, dict) and all(k in config.keys() for k in self.DEFAULT_CONFIG.keys()):
            self.config = config
        else:
            self.config = config
            for k in self.DEFAULT_CONFIG.keys():
                if k not in self.config:
                    warn("Add {!r} to config with default value {!r}".format(k, self.DEFAULT_CONFIG[k]))
                    self.config[k] = self.DEFAULT_CONFIG[k]
            config_path = base_path.joinpath(self.CONFIG_FILE)
            with config_path.open("w") as f:
                json.dump(self.config, f, sort_keys=True, indent=4)
        self.tool_folder = Path(os.path.abspath(os.path.dirname(__file__))) # type: Path
        """
        Path of the folder that this tool lays in
        """
        self.current_path = Path.cwd()
        """
        Current working directory
        """
        self.id_pattern = self.config["pattern_prefix"] + self.ID_PATTERN_MIDDLE + self.config["pattern_suffix"]
        """
        Regular expression pattern of translation ids as a string
        """
        self.id_regexp = re.compile(self.id_pattern)
        """
        Regular expression pattern of translation ids
        """
        translations = Translations.load(self.abspath(Path(self.config["translation_file"])))
        if translations is None:
            translations = Translations([], {"[add locale]": Locale("[add locale]", OrderedDict())})
        self.translations = translations # type: Translations
        self.pattern_prefix_regexp = re.compile(self.config["pattern_prefix"])
        self.pattern_suffix_regexp = re.compile(self.config["pattern_suffix"])
        self.pattern_suffix_regexp_w_end_sign = re.compile(self.config["pattern_suffix"] + "$")

    @classmethod
    def load_or_create(cls, base_path: Path) -> 'Environment':
        """
        Loads the configuration file and creates if it isn't present
        :param base_path path to work with as a base
        """
        conf = dict(cls.DEFAULT_CONFIG)
        config_path = base_path.joinpath(cls.CONFIG_FILE)
        if config_path.exists():
            with config_path.open() as f:
                conf = json.load(f) # type: dict
        else:
            with config_path.open("w") as f:
                json.dump(conf, f, sort_keys=True, indent=4)
                print("Created default configuration file at {:s}".format(str(config_path)))
        return Environment(base_path, conf)

    def abspath(self, relpath: Path) -> Path:
        return self.base_path.joinpath(relpath).absolute()

    def complete_id(self, middle_part: str) -> str:
        """
        Combines the passed middle part of a id with configured pre- and suffix to a real id.
        """
        return self.config["pattern_prefix"] + middle_part + self.config["pattern_suffix"]

    def translatable_files(self, folder=Path(".")) -> List['TranslatableFile']:
        """
        Returns the paths of all translatable files in the passed folder and its sub folders.
        :param folder: base path relative path to folder
        """
        t_files = []
        for root, dirs, files in os.walk(str(self.abspath(folder))):
            for f in files:
                if any(f.endswith(ending) for ending in self.config["translated_file_endings"]):
                    t_files.append(TranslatableFile(Path(os.path.join(root, f)), self))
        return t_files

    def find_ids(self, folder=Path(".")) -> Set[str]:
        """
        Returns the possible translation ids in all translatable files in the passed folder and its sub folders.
        :param folder: base path relative path to folder
        """
        ids = set()  # type: Set[str]
        for f in self.translatable_files(folder):
            for id in f.find_ids():
                ids.add(id)
        return ids

    def open(self, path: Path, mode: str = "r", force_target_encoding: bool = False) -> IO:
        """
        Open the path and use an appropriate encoding (the files encoding or the configured target encoding if needed)
        """
        if force_target_encoding:
            return open(str(path), mode, encoding=self.config["target_encoding"])
        # try to read the file with the default encoding
        try:
            if self.config["target_encoding"] != "utf-8":
                with open(str(path), "r") as f:
                    for l in f.read():
                        pass
            return open(str(path), mode)
        except UnicodeDecodeError:
            return self.open(path, mode, force_target_encoding=True)

    def gather_translations(self, folder=Path(".")):
        """
        Gathers new translation ids in the passed folder and its sub folders and inserts them in the current translations.
        Then stores the translations file.
        :param folder: base path relative path to folder
        """
        for id in self.find_ids(folder):
            self.translations.add_id(id)
        self.store_translations()

    def store_translations(self):
        self.translations.store(self.abspath(Path(self.config["translation_file"])))

    def normalize_id(self, id: str) -> str:
        id_wo_prefix = re.sub(self.pattern_prefix_regexp, "", id, count=1)
        id_wo_suffix = re.sub(self.pattern_suffix_regexp_w_end_sign, "", id_wo_prefix, count=1)
        return id_wo_suffix

    def post_process(self, file: Path):
        """
        Post processes the passed file after translation.
        """
        if self.config["files_to_gzip_pattern"] in str(file):
            gzipped_file_path = Path(str(file) + self.config["gzipped_file_ending"])
            with file.open("rb") as s:
                with gzip.open(gzipped_file_path, "wb") as t:
                    shutil.copyfileobj(s, t)

    def translate(self, locale: 'Locale', source_folder: Path, target_folder: Path):
        for root, dirs, files in os.walk(str(self.abspath(source_folder))):
            for f in files:
                file = Path(os.path.join(root, f))
                target_file = Path(str(file).replace(str(source_folder), str(target_folder), 1))
                os.makedirs(target_file.parent, exist_ok=True)
                if any(f.endswith(ending) for ending in self.config["translated_file_endings"]):
                    tfile = TranslatableFile(file, self)
                    tfile.translate(locale, target_file)
                else:
                    shutil.copy(file, target_file)

class Translations:
    """
    Contains the translations for different ids and locales.
    Provides a model for the underlying translation file.
    """

    def __init__(self, translation_ids: List[str], locales: Dict[str, 'Locale']):
        """
        Creates a new set of translations.
        The first locale is used as a fallback if the other locales miss a translation for a translation id
        """
        self.fallback_locale = list(locales.values())[0]
        self.locales = locales
        self.translation_ids = translation_ids
        for tid in translation_ids:
            if not self.fallback_locale.get_translation(tid).has_translation():
                raise TranslateException("Fallback locale {!r} has no translation for id {!r}"
                                         .format(self.fallback_locale.id, tid))
            for locale in self.locales.values():
                if not locale.has_translation_item(tid):
                    assert False

    @classmethod
    def load(cls, path: Path) -> Optional['Translations']:
        """
        Loads the translations from a passed csv file, returns None if the file doesn't exist

        Layout of the csv file:

        id, [fallback locale, e.g. "en"], [another locale]
        [first id, without suffix and prefix], [translation for locale], [...]
        [...]
        """
        if not path.exists():
            return None
        translation_ids = OrderedDict()  # type: OrderedDict
        locales = [] # type: List[Union[Dict[str, 'IdTranslation'], OrderedDict]]
        locale_ids = [] # type: List[str]
        try:
            with path.open() as f:
                rows = list(csv.reader(f))
                locale_ids_parts = rows[0][1:]
                assert len(locale_ids_parts) > 0
                for locale_id in locale_ids_parts:
                    locales.append(OrderedDict())
                    locale_ids.append(locale_id)
                for row in rows[1:]:
                    tid = row[0]
                    if tid in translation_ids:
                        raise "Duplicate translation id {!r}".format(tid)
                    translation_ids[tid] = 1
                    translations = row[1:]
                    translations += [""] * max(0, len(locale_ids) - len(translations))
                    for i, t in enumerate(translations):
                        locales[i][tid] = IdTranslation(tid, t, translations[0])
                locales_dict = OrderedDict()
                for i, locale in enumerate(locales):
                    locales_dict[locale_ids[i]] = Locale(locale_ids[i], locale)
                return Translations(list(translation_ids.keys()), locales_dict)
        except Exception as ex:
            raise TranslateException("The translation file has the wrong format ({!r}):\n{:s}"
                                     .format(ex, traceback.format_exc()))

    def store(self, path: Path):
        csv_rows = [["id"] + list(self.get_locale_ids())]
        for tid in self.translation_ids:
            csv_rows.append([tid] + list(l.get_translation(tid).to_csv_entry() for l in self.locales.values()))
        with path.open("w") as f:
            writer = csv.writer(f)
            for row in csv_rows:
                writer.writerow(row)

    def number_of_translations(self) -> int:
        """
        Returns the number of translation ids
        """
        return self.fallback_locale.number_of_translations()

    def get_locale(self, locale_id: str) -> 'Locale':
        if self.has_locale(locale_id):
            return self.locales[locale_id]
        raise TranslateException("No such locale {!r}".format(locale_id))

    def has_locale(self, locale_id: str) -> bool:
        return locale_id in self.locales

    def get_locale_ids(self) -> List[str]:
        return list(self.locales.keys())

    def translate(self, locale_id: str, translation_id: str) -> 'IdTranslation':
        return self.get_locale(locale_id).get_translation(translation_id)

    def contains_id(self, id: str) -> bool:
        return self.fallback_locale.has_translation_item(id)

    def add_id(self, id: str):
        """
        Adds the given translation id with empty translations for all locales if the id isn't already present
        """
        if id not in self.translation_ids:
            self.translation_ids.append(id)
            for locale in self.locales.values():
                locale.add_id(id)

class Locale:
    """
    Translation for a given locale
    """

    def __init__(self, id: str, data: Union[Dict[str, 'IdTranslation'], OrderedDict]):
        """
        Creates a new locale
        :param id: id of the locale like "en"
        :param data: list of single translations
        """
        self.id = id
        """
        Id of the locale like "en"
        """
        self._data = data
        """
        Mapping of translation ids to translations
        """

    def number_of_translations(self) -> int:
        """
        Returns the number of valid translations
        """
        return sum(1 for t in self._data.values() if t.has_translation())

    def get_translation(self, id: str) -> 'IdTranslation':
        return self._data[id]

    def untranslated_ids(self) -> List[str]:
        """
        Returns the list of translation ids without a valid translation in this locale
        """
        return [t.id for t in self._data.values() if not t.has_translation()]

    def has_translation_item(self, id: str) -> bool:
        """
        Does this locale have a translation for the given id. Even the fallback translation is enough.
        """
        return id in self._data

    def add_id(self, id: str):
        """
        Adds the given translation id with an empty translations if the id isn't already present
        """
        if id not in self._data:
            self._data[id] = IdTranslation(id, "", "")

    def translate_string(self, env: Environment, string: str, location_info: str = "") -> str:
        """
        Replaces all the ids in the string by their localized and gives
        warnings about untranslated ids.
        """
        ret_string = string
        for whole_id in re.findall(env.id_regexp, string):
            id = env.normalize_id(whole_id)
            replacement = id
            if self.has_translation_item(id):
                titem = self._data[id]
                replacement = str(titem)
                if not titem.has_translation():
                    warn("No translation for {!r} in locale {!r}, using fallback {!r}"
                         .format(id, self.id, titem.fallback))
            else:
                warn("Unknown translation id {!r} (`{:s}`){:s}"
                     .format(id, whole_id,
                             " in {:s}".format(location_info) if location_info else ""))
            ret_string = ret_string.replace(whole_id, replacement)
        return ret_string


class IdTranslation:
    """
    Translation of single translation id for a single locale
    """

    def __init__(self, id: str, translation: Optional[str], fallback: str):
        """
        Creates a new translation tuple
        :param id: id of the covered translation
        :param translation: translation or None/"", if the current locale has no translation for the id
        :param fallback: fallback translation that is used when no translation exists in the current locale
        """
        self.id = id
        """
        Translation id for which this object contains the translation
        """
        self.translation = translation if translation not in ["", None] else None  # type: Optional[str]
        """
        Translation or None, if the current locale has no translation for the id
        """
        self.fallback = fallback
        """
        Fallback translation that is used when no translation exists in the current locale
        """

    def has_translation(self) -> bool:
        return self.translation is not None

    def __str__(self):
        """
        Returns the translation or the fallback if there is no translation (and emits a warning).
        """
        if self.has_translation():
            return self.translation
        else:
            #warn("No translation for {!r} in chosen locale, using fallback {!r}".format(self.id, self.fallback))
            return self.fallback

    def to_csv_entry(self) -> str:
        return self.translation if self.has_translation() else ""


class TranslatableFile:
    """
    A translatable file with methods to translate it into a specific locale
    """

    def __init__(self, path: Path, env: Environment):
        self.path = path
        self.env = env

    def find_ids(self) -> Set[str]:
        """
        Returns the ids that match the configured pattern in the contents of this file
        """
        ret_set = set()  # type: Set[str]
        with self.env.open(self.path) as f:
            for line in f:
                for whole_id in re.findall(self.env.id_regexp, line):
                    id = self.env.normalize_id(whole_id)
                    ret_set.add(id)
        return ret_set

    def translate(self, locale: Locale, target: Path):
        """
        Translate the file into the given locale and store as the given target file.

        It also post processes the file as configured

        :param locale: locale to translate into
        :param target: given target file
        """
        if target == self.path:
            raise TranslateException("Can't override file '{:s}' during translation".format(self.path))
        with self.env.open(self.path) as s:
            with self.env.open(target, "w", force_target_encoding=True) as t:
                for line in s:
                    t.write(locale.translate_string(self.env, line, "file {!r}".format(str(self.path))))
        self.env.post_process(target)

    def __repr__(self):
        return repr(self.path)


def valid_dir_path(path: str) -> Path:
    """
    Argparse type for valid (existing) directory paths
    """
    p = Path(path)
    if not p.exists():
        raise argparse.ArgumentError("{!r} does not exist".format(path))
    if not p.is_dir():
        raise argparse.ArgumentError("{!r} is not a directory".format(path))
    return p


def valid_rel_dir_path_type(env: Environment) -> Callable[[str], Path]:
    """
    Returns an argparse type for valid (existing) directory paths relative to the base path configured
    in the passed environment
    """
    return lambda p: valid_dir_path(str(env.abspath(Path(p))))


def existing_file_type(env: Environment) -> Callable[[str], Path]:
    """
    Returns an argparse type for existing file paths relative to the base path configured in the passed
    environment
    """
    def checker(p: Path):
        path = env.abspath(Path(p))
        if not path.exists():
            raise argparse.ArgumentError("{!r} doesn't exist".format(p))
        if not path.is_file():
            raise argparse.ArgumentError("{!r} is not a file".format(p))
        return path
    return checker


def print_stats(env: Environment):
    """
    Prints statistics on the translations
    """
    translations = env.translations
    ov_translations = len(translations.translation_ids)
    print("Overall {:d} translation ids in {:d} locales".format(ov_translations, len(translations.locales)))
    print("Translation status per locale")
    for locale in translations.locales.values():
        ts = locale.number_of_translations()
        print("{:10s} {:10.0%} {:10d}".format(locale.id, ts / ov_translations, ts))


def print_stats_cli(env: Environment, parser: argparse.ArgumentParser):
    print_stats(env)


def print_untranslated(env: Environment):
    """
    Prints the untranslated ids per locale
    """
    translations = env.translations
    for locale in translations.locales.values():
        untranslated_ids = locale.untranslated_ids()
        if len(untranslated_ids) > 0:
            print("Untranslated ids for locale {:s}:".format(locale.id))
            print(", ".join(repr(id) for id in untranslated_ids))
        else:
            print("No untranslated ids for locale {:s}".format(locale.id))


def print_untranslated_cli(env: Environment, parser: argparse.ArgumentParser):
    print_untranslated(env)


def add_input_folder_option(env: Environment, parser: argparse.ArgumentParser):
    """
    Adds a FOLDER argument to the parser that requires to give a folder relative to BASE_PATH to use as an input
    """
    parser.add_argument("folder", metavar="FOLDER", help="Folder relative to BASE_PATH to use as an input",
                        type=valid_rel_dir_path_type(env))


def list_ids(env: Environment, folder: Path, only_untranslated: bool):
    ids = env.find_ids(folder)
    if only_untranslated:
        ids = set(id for id in ids if id not in env.translations.translation_ids)
    print("\n".join(sorted(ids)))


def list_ids_cli(env: Environment, parser: argparse.ArgumentParser):
    add_input_folder_option(env, parser)
    parser.add_argument("--only_untranslated", type=bool, help="Only print ids that are not already present"
                                                                " in the translation file", default=False)
    args = parser.parse_args()
    list_ids(env, args.folder, args.only_untranslated)


def gather_ids_cli(env: Environment, parser: argparse.ArgumentParser):
    add_input_folder_option(env, parser)
    args = parser.parse_args()
    env.gather_translations(args.folder)


def translate_file_cli(env: Environment, parser: argparse.ArgumentParser):
    parser.add_argument("locale", metavar="LOCALE", choices=env.translations.get_locale_ids())
    parser.add_argument("input_file", metavar="INPUT_FILE", type=existing_file_type(env))
    parser.add_argument("output_file", metavar="OUTPUT_FILE", type=lambda p: env.abspath(Path(p)))
    args = parser.parse_args()
    TranslatableFile(args.input_file, env).translate(env.translations.get_locale(args.locale), args.output_file)


def translate_cli(env: Environment, parser: argparse.ArgumentParser):
    parser.add_argument("locale", metavar="LOCALE", choices=env.translations.get_locale_ids())
    parser.add_argument("source_folder", metavar="SOURCE_FOLDER", type=valid_rel_dir_path_type(env))
    parser.add_argument("target_folder", metavar="TARGET_FOLDER", type=lambda p: env.abspath(Path(p)))
    args = parser.parse_args()
    env.translate(env.translations.get_locale(args.locale), args.source_folder, args.target_folder)


def cli():
    commands = {
        "stats": {
            "description": "prints statistics about the translate file",
            "cli": print_stats_cli
        },
        "untranslated": {
            "description": "prints the untranslated ids for each locale in the translate file",
            "cli": print_untranslated_cli
        },
        "list_ids": {
            "description": "lists the possible ids in the translatable files in the folder and its sub folders",
            "cli": list_ids_cli
        },
        "gather_ids": {
            "description": "gather the possible ids in the translatable files and uses them to update the translation "
                           "csv file",
            "cli": gather_ids_cli
        },
        "translate_file": {
            "description": "Translates and post processes a given file "
                           "(creates a gzipped version for some as configured)",
            "cli": translate_file_cli
        },
        "translate": {
            "description": "Copies all files from the SOURCE_FOLDER into the TARGET_FOLDER (paths relative to the base "
                           "path) and translates them in the process",
            "cli": translate_cli
        }
    }

    parser = argparse.ArgumentParser(description='Simple pattern based internationalization tool')
    parser.add_argument('base_path', metavar='BASE_PATH', type=valid_dir_path,
                        help="Base path that the config and translation file lie in")
    parser.add_argument('command', metavar='COMMAND', choices=sorted(commands.keys()),
                        help='Choose a command.{:s}'.format("".join(" {!r} {:s}."
                                                                           .format(c, commands[c]["description"])
                                                                           for c in commands)))
    args = parser.parse_args(sys.argv[1:3])
    if args.command not in commands:
        print("Unknown command {!r}".format(args.command))
        parser.print_help()
        exit(1)

    env = Environment.load_or_create(Path(args.base_path))
    parser.description = commands[args.command]["description"].upper()
    commands[args.command]["cli"](env, parser)

if __name__ == "__main__":
    try:
        cli()
    except TranslateException as ex:
        print(ex, file=sys.stderr)
        exit(1)