with Ada.Calendar;
with Ada.Directories;
with Ada.Environment_Variables;
with Ada.Text_IO;
procedure HostLibraryTasking is
    package D renames Ada.Directories;
    use type D.File_Size;
    package Env renames Ada.Environment_Variables;
    kEmptyTextSize : constant D.File_Size := 2; -- Empty page: LF and form feed.
    Root : constant String := "aida_phase7_tasks";
    protected Results is
        procedure Reserve (Index : out Positive);
        procedure Done (Good : Boolean);
        function Passed return Boolean;
    private
        Next : Positive := 1;
        Completed : Natural := 0;
        Errors : Natural := 0;
    end Results;
    protected body Results is
        procedure Reserve (Index : out Positive) is
        begin
            Index := Next;
            Next := Next + 1;
        end Reserve;
        procedure Done (Good : Boolean) is
        begin
            Completed := Completed + 1;
            if not Good then
                Errors := Errors + 1;
            end if;
        end Done;
        function Passed return Boolean is
        begin
            return Completed = 4 and Errors = 0;
        end Passed;
    end Results;
    procedure Exercise is
        task type Worker;
        task body Worker is
            Index : Positive;
            Visits : Natural := 0;
            Stop : exception;
            procedure Visit (Directory_Entry : D.Directory_Entry_Type) is
            begin
                if D.Simple_Name (Directory_Entry) /= "entry.txt" then
                    raise Program_Error;
                end if;
                Visits := Visits + 1;
                raise Stop;
            end Visit;
        begin
            Results.Reserve (Index);
            declare
                Name : constant String := Root & "/worker" & Positive'Image (Index);
                File : Ada.Text_IO.File_Type;
            begin
                D.Create_Directory (Name);
                Ada.Text_IO.Create (File, Name => Name & "/entry.txt");
                Ada.Text_IO.Close (File);
                for Round in 1 .. 15 loop
                    declare
                        State : D.Search_Type;
                        Item : D.Directory_Entry_Type;
                    begin
                        D.Start_Search (State, Name, "*");
                        D.Get_Next_Entry (State, Item);
                        if D.Size (Item) /= kEmptyTextSize or else not Env.Exists ("AIDA_PHASE7_SHARED")
                            or else Env.Value ("AIDA_PHASE7_SHARED") & Env.Value ("AIDA_PHASE7_SHARED") /= "sharedshared"
                            or else Ada.Calendar.Year (D.Modification_Time (Item)) < 2020 then
                            raise Program_Error;
                        end if;
                        -- Leave the active search to component finalization.
                    end;
                    begin
                        D.Search (Name, "", Process => Visit'Access);
                    exception
                        when Stop => null;
                    end;
                end loop;
                if Visits /= 15 then
                    raise Program_Error;
                end if;
                D.Delete_Tree (Name);
            end;
            Results.Done (True);
        exception
            when others => Results.Done (False);
        end Worker;
        type Group is array (1 .. 4) of Worker;
        Workers : Group;
    begin
        null;
    end Exercise;
begin
    if D.Exists (Root) then
        D.Delete_Tree (Root);
    end if;
    D.Create_Directory (Root);
    Env.Set ("AIDA_PHASE7_SHARED", "shared");
    Exercise;
    Env.Clear ("AIDA_PHASE7_SHARED");
    D.Delete_Tree (Root);
    if not Results.Passed then
        raise Program_Error;
    end if;
    Ada.Text_IO.Put_Line ("host library tasking passed");
end HostLibraryTasking;
