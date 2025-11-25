# Permission Set Comparator (C++ / Qt6)

A small native desktop tool to compare Salesforce permission sets between two users and list permission sets that the mirror user has but the primary user lacks.
<img width="902" height="832" alt="Permission Set Comparator" src="https://github.com/user-attachments/assets/ea171de1-9d1b-4525-887b-92ab64a7adac" />

## Overview

- Language: C++17
- Framework: Qt6 (Widgets)
- Build: CMake + Ninja (tested with MinGW on Windows)

The application reads a CSV of permission set metadata (name/label/description) and uses it to show friendly descriptions next to missing permission sets.

## Important: Supply Your Own CSV

You must provide your own permission set CSV file. The application expects a CSV with the following columns (the bundled CSV-cleanup step supports common variations):

- `Id` — (optional) Salesforce permission set Id
- `Name` — API name (this is used to match permission set names in the UI)
- `Label` — Human-friendly label
- `Description` — Full description text

Save the CSV as `Permission Sets.csv` and place it in the same folder as the executable before running.

Example CSV snippet:

```
Id,Name,Label,Description
0PS...,Lightning_Experience_User,Lightning Experience User,Enables the Lightning Experience User permission.
```

## Building (Windows)

1. Install Qt 6 (with MinGW) and Ninja.
2. Open a Developer PowerShell and run from the project root:

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="C:/Qt/6.xx.x/mingw_64"
cmake --build build
```

3. Copy your `Permission Sets.csv` and icon into the `build` or output folder alongside `SalesforcePermCalc.exe`.

4. To create a distributable package on Windows, run `windeployqt` on the built executable (Qt's `bin` folder):

```powershell
C:\Qt\6.xx.x\mingw_64\bin\windeployqt.exe --release --no-translations --compiler-runtime "path\to\SalesforcePermCalc.exe"
```

## Usage

1. Run `SalesforcePermCalc.exe`.
2. Paste the primary user's permission set names into the left box.
3. Paste the mirror user's permission set names into the right box.
4. Click **Compare Permissions**. Missing permission sets (mirror minus primary) will appear with descriptions.

Notes:
- The app performs tolerant parsing of pasted text — it accepts tab, comma, multi-space, and line-separated lists.
- Matching is case-insensitive.

## Distribution

Place the executable, `Permission Sets.csv`, and the Qt DLLs produced by `windeployqt` into a folder and zip it for distribution.

## Files of Interest

- `perm_set_calculator.cpp` — Application source and UI
- `CMakeLists.txt` — Build setup
- `Permission Sets.csv` — Permission set metadata (user-provided)

## License

Add a license of your choice. (I can add an MIT or Apache-2.0 license if you'd like.)

---

If you want, I can also create a zipped release inside `permission_set_comparator`.
