"""A small editor for Quantum Redshift profiles (tools\\saves\\profiles.py): open a save folder or a viewer JSON profile,
edit it, save it as a signed save folder or as viewer JSON.

usage: python tools\\saves\\profile_editor.py [save folder or profile .json]

The signing key file's path (never the key) and the XBE path are remembered in %APPDATA%\\qr_profile\\settings.json.
"""
import argparse
import copy
import json
import os
import sys
import tkinter as tk
from pathlib import Path
from tkinter import filedialog, messagebox, ttk

sys.path.insert(0, str(Path(__file__).resolve().parent))   # profiles.py beside it, also under python -I
import profiles as qp  # noqa: E402

PAD = {'padx': 4, 'pady': 2}


class UserSettings:
    """Per-user paths for the GUI, outside the repository."""

    def __init__(self):
        self.path = Path(os.environ.get('APPDATA', Path.home())) / 'qr_profile' / 'settings.json'
        self.values = self._read()

    def _read(self):
        try:
            return json.loads(self.path.read_text(encoding='utf-8'))
        except (OSError, ValueError):
            return {}

    def get(self, key):
        return self.values.get(key, '')

    def remember(self, key, value):
        self.values[key] = value
        self.path.parent.mkdir(parents=True, exist_ok=True)
        self.path.write_text(json.dumps(self.values, indent=2), encoding='utf-8')


# ---- fields: each shows one part of the save and writes it back only when the user changed it

class FlagsField:
    """A list of byte flags (or one bit mask) as checkboxes."""

    def __init__(self, attribute, count, is_mask=False):
        self.attribute = attribute
        self.is_mask = is_mask
        self.variables = [tk.BooleanVar() for _ in range(count)]
        self.loaded = []

    def _flags(self, owner):
        value = getattr(owner, self.attribute)
        return qp.mask_to_flags(value, len(self.variables)) if self.is_mask else value

    def load(self, owner):
        for variable, flag in zip(self.variables, self._flags(owner)):
            variable.set(bool(flag))
        self.loaded = [variable.get() for variable in self.variables]

    def store(self, owner):
        if self.is_mask:
            self._store_mask(owner)
        else:
            self._store_flags(owner)

    def _store_flags(self, owner):
        flags = getattr(owner, self.attribute)
        for i, (variable, before) in enumerate(zip(self.variables, self.loaded)):
            if variable.get() != before:
                flags[i] = int(variable.get())

    def _store_mask(self, owner):
        mask = getattr(owner, self.attribute)
        for i, (variable, before) in enumerate(zip(self.variables, self.loaded)):
            if variable.get() != before:
                mask = mask | 1 << i if variable.get() else mask & ~(1 << i)
        setattr(owner, self.attribute, mask)

    def changed(self, index):
        return self.variables[index].get() != self.loaded[index]


class ByteFlagField:
    """One byte flag (Mars, the cheat menu) as a checkbox."""

    def __init__(self, attribute):
        self.attribute = attribute
        self.variables = [tk.BooleanVar()]
        self.loaded = False

    def load(self, owner):
        self.variables[0].set(bool(getattr(owner, self.attribute)))
        self.loaded = self.variables[0].get()

    def store(self, owner):
        if self.variables[0].get() != self.loaded:
            setattr(owner, self.attribute, int(self.variables[0].get()))


class ChoiceField:
    """An int shown as one of a list of names."""

    def __init__(self, attribute, names):
        self.attribute = attribute
        self.names = list(names)
        self.variable = tk.StringVar()
        self.loaded = ''

    def load(self, owner):
        value = getattr(owner, self.attribute)
        self.variable.set(self.names[value] if 0 <= value < len(self.names) else str(value))
        self.loaded = self.variable.get()

    def chosen(self):
        return self.names.index(self.variable.get())

    def is_changed(self):
        return self.variable.get() != self.loaded and self.variable.get() in self.names

    def store(self, owner):
        if self.is_changed():
            setattr(owner, self.attribute, self.chosen())


class NumberField:
    """An int typed in an entry or a spinbox."""

    def __init__(self, attribute, label):
        self.attribute = attribute
        self.label = label
        self.variable = tk.StringVar()
        self.loaded = ''

    def load(self, owner):
        self.variable.set(str(getattr(owner, self.attribute)))
        self.loaded = self.variable.get()

    def store(self, owner):
        if self.variable.get() == self.loaded:
            return
        try:
            setattr(owner, self.attribute, int(self.variable.get()))
        except ValueError as error:
            raise ValueError('%s: %r is not a whole number' % (self.label, self.variable.get())) from error


class ListNumberField(NumberField):
    """One value of a list attribute (an upgrade level)."""

    def __init__(self, attribute, index, label):
        super().__init__(attribute, label)
        self.index = index

    def load(self, owner):
        self.variable.set(str(getattr(owner, self.attribute)[self.index]))
        self.loaded = self.variable.get()

    def store(self, owner):
        if self.variable.get() == self.loaded:
            return
        value = int(self.variable.get()) if self.variable.get().isdigit() else -1
        if not 0 <= value <= qp.MAX_UPGRADE:
            raise ValueError('%s: the level must be 0..%d' % (self.label, qp.MAX_UPGRADE))
        getattr(owner, self.attribute)[self.index] = value


class SettingField:
    """One Game / Sound Settings entry."""

    def __init__(self, entry):
        self.entry = entry
        self.variable = tk.StringVar()
        self.loaded = ''

    def load(self, settings):
        value = next(value for entry, value, _, _ in settings.choice_entries() if entry is self.entry)
        self.variable.set(self.entry.choices[value] if value < len(self.entry.choices) else str(value))
        self.loaded = self.variable.get()

    def store(self, settings):
        if self.variable.get() != self.loaded and self.variable.get() in self.entry.choices:
            settings.set_choice(self.entry.type_id, self.entry.choices.index(self.variable.get()))


class OpenClassField(ChoiceField):
    """The open speed class: the game opens the tournament level with it."""

    def store(self, owner):
        if self.is_changed():
            owner.set_open_class(self.chosen())


class CheatsOnField(FlagsField):
    """Cheats 5 / 6 are stored twice (the activated byte and the switch); both follow the checkbox."""

    def load(self, owner):
        mask = qp.cheats_on_mask(owner)
        for i, variable in enumerate(self.variables):
            variable.set(bool(mask >> i & 1))
        self.loaded = [variable.get() for variable in self.variables]

    def store(self, owner):
        self._store_flags(owner)
        if self.changed(qp.INFINITE_SHIELD_CHEAT):
            owner.infinite_shield = int(self.variables[qp.INFINITE_SHIELD_CHEAT].get())
        if self.changed(qp.INFINITE_TURBO_CHEAT):
            owner.infinite_turbo = int(self.variables[qp.INFINITE_TURBO_CHEAT].get())


# ---- the window

class ProfileEditorApp:
    def __init__(self, root):
        self.root = root
        self.settings = UserSettings()
        self.save = qp.new_save('PLAYER1')
        self.source = 'new profile'
        self.save_folder = None   # the save folder the open profile came from (or was last saved to)
        self.name = tk.StringVar()
        self.status = tk.StringVar()
        self.percent = tk.StringVar()
        self.profile_fields = []
        self.record_fields = [[] for _ in range(qp.CHARACTERS)]
        self.setting_fields = []
        root.title('Quantum Redshift profile editor')
        self._build_toolbar()
        self._build_tabs()
        ttk.Label(root, textvariable=self.status, anchor='w').pack(fill='x', **PAD)
        self.show(self.save, self.source)

    # -- layout

    def _build_toolbar(self):
        bar = ttk.Frame(self.root)
        bar.pack(fill='x', **PAD)
        for text, command in (('Open save folder...', self.open_save_folder), ('Open viewer JSON...', self.open_viewer_json),
                              ('New', self.new_profile), ('Save as save folder...', self.save_as_folder),
                              ('Save as viewer JSON...', self.save_as_json),
                              ('keys.json...', self.choose_keys_json),
                              ('Game key file...', self.choose_signature_key_file), ('XboxCERTKey file...', self.choose_key_file),
                              ('default.xbe...', self.choose_xbe)):
            ttk.Button(bar, text=text, command=command).pack(side='left', **PAD)
        ttk.Label(bar, textvariable=self.percent, font=('TkDefaultFont', 11, 'bold')).pack(side='right', **PAD)

    def _build_tabs(self):
        tabs = ttk.Notebook(self.root)
        tabs.pack(fill='both', expand=True, **PAD)
        for title, build in (('Profile', self._build_profile_tab), ('Unlocks', self._build_unlocks_tab),
                             ('Characters', self._build_characters_tab), ('Eggs and cheats', self._build_eggs_tab),
                             ('Settings', self._build_settings_tab)):
            frame = ttk.Frame(tabs)
            tabs.add(frame, text=title)
            build(frame)

    def _add_profile_field(self, field):
        self.profile_fields.append(field)
        return field

    def _checkboxes(self, parent, title, field, names, columns=2):
        box = ttk.LabelFrame(parent, text=title)
        for i, (variable, name) in enumerate(zip(field.variables, names)):
            ttk.Checkbutton(box, text=name, variable=variable, command=self.update_percent).grid(
                row=i // columns, column=i % columns, sticky='w', **PAD)
        return box

    def _combobox(self, parent, row, label, field):
        ttk.Label(parent, text=label).grid(row=row, column=0, sticky='w', **PAD)
        ttk.Combobox(parent, textvariable=field.variable, values=field.names, state='readonly', width=24).grid(
            row=row, column=1, sticky='w', **PAD)

    def _build_profile_tab(self, frame):
        ttk.Label(frame, text='Name (1..9 characters)').grid(row=0, column=0, sticky='w', **PAD)
        ttk.Entry(frame, textvariable=self.name, width=26).grid(row=0, column=1, sticky='w', **PAD)
        self._combobox(frame, 1, 'Speed classes open up to',
                       self._add_profile_field(OpenClassField('speed_class', qp.LEVEL_NAMES)))
        self._combobox(frame, 2, 'Last tournament level started',
                       self._add_profile_field(ChoiceField('tourney_level', qp.LEVEL_NAMES)))
        self._combobox(frame, 3, 'Music source',
                       self._add_profile_field(ChoiceField('music_default', ('Custom', 'Default'))))
        wins = self._add_profile_field(FlagsField('redshift_wins', len(qp.REDSHIFT_WIN_NAMES)))
        self._checkboxes(frame, 'Redshift wins', wins, qp.REDSHIFT_WIN_NAMES, columns=1).grid(
            row=4, column=0, columnspan=2, sticky='we', **PAD)

    def _build_unlocks_tab(self, frame):
        characters = self._add_profile_field(FlagsField('characters', qp.CHARACTERS, is_mask=True))
        self._checkboxes(frame, 'Characters', characters, qp.CHARACTER_NAMES).grid(row=0, column=0, sticky='nw', **PAD)
        tracks = self._add_profile_field(FlagsField('tracks', 16, is_mask=True))
        self._checkboxes(frame, 'Tracks', tracks, qp.TRACK_NAMES[:16]).grid(row=0, column=1, sticky='nw', **PAD)
        mars = self._add_profile_field(ByteFlagField('mars'))
        self._checkboxes(frame, 'Mars', mars, ('Mars unlocked',)).grid(row=1, column=1, sticky='nw', **PAD)

    def _build_characters_tab(self, frame):
        ttk.Button(frame, text='All tournaments won', command=self.win_all_tournaments).grid(
            row=0, column=0, columnspan=3, sticky='w', **PAD)
        headings = ('Character', 'Cash') + qp.UPGRADE_NAMES + tuple('%s trophy' % level for level in qp.LEVEL_NAMES)
        for column, heading in enumerate(headings):
            ttk.Label(frame, text=heading).grid(row=1, column=column, **PAD)
        for c, name in enumerate(qp.CHARACTER_NAMES):
            self._build_character_row(frame, c + 2, c, name)

    def _build_character_row(self, frame, row, c, name):
        ttk.Label(frame, text=name).grid(row=row, column=0, sticky='w', **PAD)
        cash = NumberField('cash', '%s cash' % name)
        ttk.Entry(frame, textvariable=cash.variable, width=10).grid(row=row, column=1, **PAD)
        fields = [cash]
        for k, upgrade in enumerate(qp.UPGRADE_NAMES):
            level = ListNumberField('upgrades', k, '%s %s' % (name, upgrade))
            ttk.Spinbox(frame, textvariable=level.variable, from_=0, to=qp.MAX_UPGRADE, width=3).grid(
                row=row, column=2 + k, **PAD)
            fields.append(level)
        trophies = FlagsField('trophies', qp.LEVELS)
        for level, variable in enumerate(trophies.variables):
            ttk.Checkbutton(frame, variable=variable, command=self.update_percent).grid(row=row, column=5 + level)
        fields.append(trophies)
        self.record_fields[c] = fields

    def _build_eggs_tab(self, frame):
        eggs = self._add_profile_field(FlagsField('eggs', qp.EGGS))
        self._checkboxes(frame, 'Easter eggs unlocked', eggs, qp.EGG_NAMES, columns=1).grid(row=0, column=0, sticky='nw', **PAD)
        eggs_on = self._add_profile_field(FlagsField('eggs_on', qp.EGGS))
        self._checkboxes(frame, 'Easter eggs on', eggs_on, qp.EGG_NAMES, columns=1).grid(row=0, column=1, sticky='nw', **PAD)
        cheats = self._add_profile_field(FlagsField('cheats', qp.CHEATS))
        self._checkboxes(frame, 'Cheats unlocked', cheats, qp.CHEAT_NAMES, columns=1).grid(row=1, column=0, sticky='nw', **PAD)
        cheats_on = self._add_profile_field(CheatsOnField('cheats_on', qp.CHEATS))
        self._checkboxes(frame, 'Cheats activated', cheats_on, qp.CHEAT_NAMES, columns=1).grid(
            row=1, column=1, sticky='nw', **PAD)
        menu = self._add_profile_field(ByteFlagField('cheat_menu'))
        self._checkboxes(frame, 'Cheat menu', menu, ('Cheat menu shown in Options',)).grid(row=2, column=0, sticky='nw', **PAD)

    def _build_settings_tab(self, frame):
        rows = [entry for entry in qp.GAME_SETTINGS + qp.SOUND_SETTINGS if entry.choices]
        for row, entry in enumerate(rows):
            field = SettingField(entry)
            ttk.Label(frame, text=entry.label).grid(row=row, column=0, sticky='w', **PAD)
            ttk.Combobox(frame, textvariable=field.variable, values=entry.choices, state='readonly', width=12).grid(
                row=row, column=1, sticky='w', **PAD)
            self.setting_fields.append(field)

    # -- the save <-> the fields

    def show(self, save, source):
        self.save = save
        self.source = source
        self.name.set(save.name)
        for field in self.profile_fields:
            field.load(save.profile)
        for record, fields in zip(save.profile.records, self.record_fields):
            for field in fields:
                field.load(record)
        for field in self.setting_fields:
            field.load(save.settings)
        self.update_percent()
        key_source, key = self.chosen_signing_key()
        self.status.set('%s  |  signature as loaded: %s  |  key: %s' % (source, key.check(save).value, key_source))

    def edited_save(self):
        """A copy of the save with the fields' changes (the open one stays as loaded until it is saved)."""
        save = copy.deepcopy(self.save)
        if self.name.get() != save.name:
            save.rename(self.name.get())
        for field in self.profile_fields:
            field.store(save.profile)
        for record, fields in zip(save.profile.records, self.record_fields):
            for field in fields:
                field.store(record)
        for field in self.setting_fields:
            field.store(save.settings)
        return save

    def win_all_tournaments(self):
        """The edits so far plus every tournament won (trophies and what they open); saved only with Save as..."""
        try:
            save = self.edited_save()
        except ValueError as error:
            messagebox.showerror('All tournaments won', str(error))
            return
        save.profile.win_all_tournaments()
        mark = ' (every tournament won, not saved yet)'
        self.show(save, self.source if self.source.endswith(mark) else self.source + mark)

    def update_percent(self):
        try:
            self.percent.set('Percentage complete: %d%%' % self.edited_save().profile.percent_complete())
        except ValueError:
            self.percent.set('Percentage complete: ?')

    # -- commands

    def key_files(self):
        """The remembered paths; a .json picked as a key file is a keys.json and is used as one."""
        keys_json = self.settings.get('keys_json') or str(qp.KEYS_JSON)
        key_files = []
        for setting in ('signature_key_file', 'cert_key_file'):
            path = self.settings.get(setting)
            if path.lower().endswith('.json'):
                keys_json, path = path, ''
            key_files.append(path)
        return qp.KeyFiles(key_files[0], key_files[1], self.settings.get('xbe_file'), keys_json)

    def chosen_signing_key(self):
        try:
            return qp.chosen_signing_key(self.key_files())
        except (OSError, ValueError) as error:
            messagebox.showerror('Signing key', str(error))
            return 'no key (%s)' % error, qp.MissingSigningKey()

    def signing_key(self):
        return self.chosen_signing_key()[1]

    def open_path(self, path):
        path = Path(path)
        try:
            if path.suffix.lower() == '.json':
                save, folder = qp.from_viewer_json(qp.load_viewer_json(path)), None
            else:
                save, folder = qp.load_save(path), qp.save_file_path(path).parent
        except (OSError, ValueError) as error:
            messagebox.showerror('Open', str(error))
            return
        self.save_folder = folder
        self.show(save, str(path))

    def open_save_folder(self):
        folder = filedialog.askdirectory(title='Save folder (holds savegame.sav)', initialdir=self.settings.get('last_folder'))
        if folder:
            self.settings.remember('last_folder', str(Path(folder).parent))
            self.open_path(folder)

    def open_viewer_json(self):
        path = filedialog.askopenfilename(title='Viewer profile', filetypes=(('Profile JSON', '*.json'),))
        if path:
            self.open_path(path)

    def new_profile(self):
        self.save_folder = None
        self.show(qp.new_save('PLAYER1'), 'new profile')

    def target_folder(self, chosen, save):
        """Where the profile goes: the folder the game looks it up in, <id> = the hash of its name. The chosen folder
        itself when it is that one; next to it when it is another save folder (the profile renamed); else inside it."""
        chosen = Path(chosen)
        folder_id = qp.profile_folder_id(save.name)
        if chosen.name.upper() == folder_id:
            return chosen
        if (chosen / qp.META_FILE).is_file():
            return chosen.parent / folder_id
        return chosen / folder_id

    def save_as_folder(self):
        try:
            save = self.edited_save()
        except ValueError as error:
            messagebox.showerror('Save', str(error))
            return
        signing_key = self.signing_key()
        if not signing_key.available and not messagebox.askyesno(
                'Not signed', 'No signing key file is set: savegame.sav keeps its old signature and the game will list '
                              'an edited profile as "Invalid profile". Save anyway?'):
            return
        chosen = filedialog.askdirectory(title='Folder to save into (the profile goes in its <id> folder)',
                                         initialdir=self.settings.get('last_folder'))
        if chosen:
            self.write_folder(save, signing_key, self.target_folder(chosen, save))

    def write_folder(self, save, signing_key, folder):
        """savegame.sav and SaveMeta.xbx (Name = the profile's name); a renamed profile may leave its old folder."""
        previous_name, previous_folder = self.save.name, self.save_folder
        try:
            if signing_key.available:
                save.sign(signing_key)
            problems = qp.write_save_folder(save, folder, previous_name)
        except (OSError, ValueError) as error:
            messagebox.showerror('Save', str(error))
            return
        self.save_folder = Path(folder)
        self.show(save, str(folder))
        notes = [str(folder), 'SaveMeta.xbx: Name=%s' % save.name, 'signed' if signing_key.available else 'not signed']
        messagebox.showinfo('Saved', '\n'.join(notes + problems))
        if previous_folder and Path(previous_folder) != Path(folder):
            self.offer_to_remove(previous_folder, previous_name)

    def offer_to_remove(self, folder, name):
        """The renamed profile's old folder: the game would still list it as its old name."""
        folder = Path(folder)
        meta = folder / qp.META_FILE
        if not meta.is_file() or qp.save_meta_name(meta.read_bytes()) != qp.profile_save_name(name):
            return
        if not messagebox.askyesno('Renamed', 'The profile was saved under its new name. Remove the old save folder\n'
                                              '%s (profile %r)?' % (folder, name)):
            return
        try:
            qp.remove_save_folder(folder)
        except OSError as error:
            messagebox.showerror('Remove', str(error))

    def save_as_json(self):
        try:
            save = self.edited_save()
        except ValueError as error:
            messagebox.showerror('Save', str(error))
            return
        base = ''.join(c if c.isalnum() else '_' for c in save.name) or 'profile'
        path = filedialog.asksaveasfilename(title='Viewer profile', defaultextension='.json', initialfile=base + '.json',
                                            filetypes=(('Profile JSON', '*.json'),))
        if path:
            Path(path).write_text(qp.viewer_json_text(qp.to_viewer_json(save)), encoding='utf-8')
            self.show(save, path)

    def choose_keys_json(self):
        """keys.json (XboxSignatureKey / XboxCERTKey / XbeSignature) becomes the key source: the key files chosen
        before are forgotten, as they would come first."""
        path = filedialog.askopenfilename(title='keys.json', filetypes=(('Keys', '*.json'),),
                                          initialdir=str(Path(self.key_files().keys_json).parent))
        if path:
            self.settings.remember('keys_json', path)
            self.settings.remember('signature_key_file', '')
            self.settings.remember('cert_key_file', '')
            self.show(self.save, self.source)

    def choose_signature_key_file(self):
        path = filedialog.askopenfilename(title="The game's signing key file (tools\\saves\\signature_key.py, 32 hex digits)")
        if path:
            self.settings.remember('signature_key_file', path)
            self.show(self.save, self.source)

    def choose_key_file(self):
        path = filedialog.askopenfilename(title='XboxCERTKey file (32 hex digits)')
        if path:
            self.settings.remember('cert_key_file', path)
            self.show(self.save, self.source)

    def choose_xbe(self):
        path = filedialog.askopenfilename(title="The game's default.xbe", filetypes=(('XBE', '*.xbe'),))
        if path:
            self.settings.remember('xbe_file', path)
            self.show(self.save, self.source)


def parse_arguments():
    parser = argparse.ArgumentParser(description='A small editor for Quantum Redshift profiles: open a save folder or a '
                                                 'viewer JSON profile, edit it, save it as a signed save folder or as viewer JSON.')
    parser.add_argument('profile', nargs='?', help='a save folder or a profile .json to open')
    return parser.parse_args()


def main():
    arguments = parse_arguments()
    root = tk.Tk()
    app = ProfileEditorApp(root)
    if arguments.profile:
        app.open_path(arguments.profile)
    root.mainloop()


if __name__ == '__main__':
    main()
