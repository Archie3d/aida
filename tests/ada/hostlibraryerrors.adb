with Ada.Calendar;
with Ada.Directories;
with Ada.Environment_Variables;
procedure HostLibraryErrors is
    A, B : Ada.Directories.Search_Type;
    X, Y : Ada.Directories.Directory_Entry_Type;
    T : Ada.Calendar.Time := 0;
begin
    A := B;
    X := Y;
end HostLibraryErrors;
