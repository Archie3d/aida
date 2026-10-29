with Ada.Calendar;
with Ada.Directories;
with Ada.Text_IO;
procedure DirectoriesLibrary is
    use Ada.Directories;
    use type Ada.Calendar.Time;
    Root : constant String := "aida_phase7_directories";
    Count : Natural := 0;
    State : Search_Type;
    Item : Directory_Entry_Type;
    File : Ada.Text_IO.File_Type;
    Caught : Boolean := False;
    procedure Check (Value : Boolean) is
    begin
        if not Value then
            raise Program_Error;
        end if;
    end Check;
    procedure Visit (Directory_Entry : Directory_Entry_Type) is
    begin
        Check (Kind (Directory_Entry) = Ordinary_File);
        Check (Simple_Name (Directory_Entry) = "renamed.txt");
        Check (Size (Directory_Entry) = 6);
        Count := Count + 1;
    end Visit;
begin
    if Exists (Root) then
        Delete_Tree (Root);
    end if;
    Check (not More_Entries (State));
    begin
        Check (Kind (Item) = Directory);
    exception
        when Status_Error => Caught := True;
    end;
    Check (Caught);
    Create_Path (Root & "/one/two");
    Create_Path (Root & "/one/two");
    Check (Kind (Root) = Directory);
    Ada.Text_IO.Create (File, Name => Root & "/hello.txt");
    Ada.Text_IO.Put_Line (File, "hello");
    Ada.Text_IO.Close (File);
    Check (Exists (Root & "/hello.txt") and Size (Root & "/hello.txt") = 6);
    Copy_File (Root & "/hello.txt", Root & "/copy.txt");
    Check (Size (Root & "/copy.txt") = 6);
    Rename (Root & "/copy.txt", Root & "/renamed.txt");
    Check (not Exists (Root & "/copy.txt"));
    Caught := False;
    begin
        Rename (Root & "/hello.txt", Root & "/renamed.txt");
    exception
        when Use_Error => Caught := True;
    end;
    Check (Caught and Size (Root & "/renamed.txt") = 6);
    Caught := False;
    begin
        Rename (Root & "/hello.txt", Root & "/missing/new.txt");
    exception
        when Use_Error => Caught := True;
    end;
    Check (Caught and Exists (Root & "/hello.txt"));
    Caught := False;
    begin
        Copy_File (Root & "/hello.txt", Root & "/hello.txt");
    exception
        when Use_Error => Caught := True;
    end;
    Check (Caught and Size (Root & "/hello.txt") = 6);
    Check (Ada.Calendar.Year (Modification_Time (Root & "/hello.txt")) >= 2020);
    Check (Simple_Name (Root & "/hello.txt") = "hello.txt");
    Check (Base_Name (Root & "/hello.txt") = "hello");
    Check (Extension (Root & "/hello.txt") = "txt");
    Check (Containing_Directory (Root & "/hello.txt") = Root);
    Check (Containing_Directory ("hello.txt") = ".");
    Check (Extension (".hidden") = "");
    Check (Compose (Root, "hello", "txt") = Root & "/hello.txt");
    Check (Full_Name ("missing/../file.txt") = Current_Directory & "/file.txt");
    Start_Search (State, Root, "*.txt", (Ordinary_File => True, others => False));
    Check (More_Entries (State));
    Get_Next_Entry (State, Item);
    Check (Kind (Item) = Ordinary_File and Size (Item) = 6);
    Check (Modification_Time (Item) <= Ada.Calendar.Clock);
    End_Search (State);
    End_Search (State);
    Check (not More_Entries (State) and Size (Item) = 6);
    Search (Root, "renamed.?xt", Process => Visit'Access);
    Check (Count = 1);
    Start_Search (State, Root, "", (others => False));
    Check (not More_Entries (State));
    Caught := False;
    begin
        Get_Next_Entry (State, Item);
    exception
        when Status_Error => Caught := True;
    end;
    Check (Caught);
    Caught := False;
    begin
        Start_Search (State, Root & "/missing", "*");
    exception
        when Name_Error => Caught := True;
    end;
    Check (Caught and not More_Entries (State));
    Caught := False;
    begin
        Create_Directory (Root & "/unsupported", "ignored=no");
    exception
        when Use_Error => Caught := True;
    end;
    Check (Caught and not Exists (Root & "/unsupported"));
    declare
        Saved : constant String := Current_Directory;
    begin
        Set_Directory (Root);
        Check (Exists ("hello.txt"));
        Set_Directory (Saved);
    end;
    Caught := False;
    begin
        Search (Root, "*", Process => null);
    exception
        when Constraint_Error => Caught := True;
    end;
    Check (Caught);
    Delete_File (Root & "/hello.txt");
    Delete_Directory (Root & "/one/two");
    Delete_Tree (Root);
    Check (not Exists (Root));
    Ada.Text_IO.Put_Line ("directories library passed");
end DirectoriesLibrary;
