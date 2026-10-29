package body Ada.Directories is
    use type System.Address;
    function Start (Directory, Pattern : String; Mask : Integer) return System.Address;
    pragma Import (C, Start, "__ada_host_search_start");
    procedure Release (Handle : System.Address);
    pragma Import (C, Release, "__ada_host_search_release");
    function More (Handle : System.Address) return Boolean;
    pragma Import (C, More, "__ada_host_search_more");
    function Next_Name (Handle : System.Address) return String;
    pragma Import (C, Next_Name, "__ada_host_search_name");
    function Next_Kind (Handle : System.Address) return File_Kind;
    pragma Import (C, Next_Kind, "__ada_host_search_kind");
    function Next_Size (Handle : System.Address) return File_Size;
    pragma Import (C, Next_Size, "__ada_host_search_size");
    procedure Next_Time (Handle : System.Address; Year, Month, Day : out Integer;
                         Seconds : out Duration; Valid : out Boolean);
    pragma Import (C, Next_Time, "__ada_host_search_time");
    procedure Step (Handle : System.Address);
    pragma Import (C, Step, "__ada_host_search_step");
    procedure File_Time (Name : String; Year, Month, Day : out Integer; Seconds : out Duration);
    pragma Import (C, File_Time, "__ada_host_modification_time");
    overriding procedure Finalize (Object : in out Search_Guard) is
        Handle : constant System.Address := Object.Handle;
    begin
        Object.Handle := null;
        Release (Handle);
    end Finalize;
    function Modification_Time (Name : String) return Ada.Calendar.Time is
        Y, M, D : Integer;
        S : Duration;
    begin
        File_Time (Name, Y, M, D, S);
        return Ada.Calendar.Time_Of (Y, M, D, S);
    end Modification_Time;
    procedure Start_Search (Search : in out Search_Type; Directory, Pattern : String;
                            Filter : Filter_Type := (others => True)) is
        Mask : Integer := 0;
    begin
        End_Search (Search);
        if Filter (Ada.Directories.Directory) then
            Mask := Mask + 1;
        end if;
        if Filter (Ordinary_File) then
            Mask := Mask + 2;
        end if;
        if Filter (Special_File) then
            Mask := Mask + 4;
        end if;
        Search.State.Handle := Start (Directory, Pattern, Mask);
    end Start_Search;
    procedure End_Search (Search : in out Search_Type) is
    begin
        Finalize (Search.State);
    end End_Search;
    function More_Entries (Search : Search_Type) return Boolean is
    begin
        return More (Search.State.Handle);
    end More_Entries;
    procedure Get_Next_Entry (Search : in out Search_Type; Directory_Entry : out Directory_Entry_Type) is
        Y, M, D : Integer;
        S : Duration;
    begin
        if not More_Entries (Search) then
            raise Status_Error;
        end if;
        Directory_Entry.Valid := False;
        Directory_Entry.Name := Ada.Strings.Unbounded.To_Unbounded_String (Next_Name (Search.State.Handle));
        Directory_Entry.File_Type := Next_Kind (Search.State.Handle);
        Directory_Entry.Length := Next_Size (Search.State.Handle);
        Next_Time (Search.State.Handle, Y, M, D, S, Directory_Entry.Has_Time);
        if Directory_Entry.Has_Time then
            Directory_Entry.Modified := Ada.Calendar.Time_Of (Y, M, D, S);
        end if;
        Directory_Entry.Valid := True;
        Step (Search.State.Handle);
    end Get_Next_Entry;
    procedure Search (Directory, Pattern : String; Filter : Filter_Type := (others => True);
                      Process : not null access procedure (Directory_Entry : Directory_Entry_Type)) is
        State : Search_Type;
        Item : Directory_Entry_Type;
    begin
        Start_Search (State, Directory, Pattern, Filter);
        while More_Entries (State) loop
            Get_Next_Entry (State, Item);
            Process (Item);
        end loop;
    end Search;
    procedure Check (Item : Directory_Entry_Type) is
    begin
        if not Item.Valid then
            raise Status_Error;
        end if;
    end Check;
    function Full_Name (Directory_Entry : Directory_Entry_Type) return String is
    begin
        Check (Directory_Entry);
        return Ada.Strings.Unbounded.To_String (Directory_Entry.Name);
    end Full_Name;
    function Simple_Name (Directory_Entry : Directory_Entry_Type) return String is
    begin
        return Simple_Name (Full_Name (Directory_Entry));
    end Simple_Name;
    function Kind (Directory_Entry : Directory_Entry_Type) return File_Kind is
    begin
        Check (Directory_Entry);
        return Directory_Entry.File_Type;
    end Kind;
    function Size (Directory_Entry : Directory_Entry_Type) return File_Size is
    begin
        Check (Directory_Entry);
        return Directory_Entry.Length;
    end Size;
    function Modification_Time (Directory_Entry : Directory_Entry_Type) return Ada.Calendar.Time is
    begin
        Check (Directory_Entry);
        if not Directory_Entry.Has_Time then
            raise Use_Error;
        end if;
        return Directory_Entry.Modified;
    end Modification_Time;
end Ada.Directories;
