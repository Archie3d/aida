with Ada.Text_IO;

package body Report is
    Has_Failed : Boolean := False;
    Test_Name : String (1 .. 7) := "NO_NAME";

    procedure Test (Name : String; Description : String) is
    begin
        Has_Failed := False;
        Test_Name := (others => '_');
        for Index in 1 .. 7 loop
            exit when Index > Name'Length;
            Test_Name (Index) := Name (Name'First + Index - 1);
        end loop;
        Ada.Text_IO.Put_Line ("TEST " & Name);
    end Test;

    procedure Failed (Description : String) is
    begin
        Has_Failed := True;
        Ada.Text_IO.Put_Line ("FAILED " & Description);
    end Failed;

    procedure Result is
    begin
        if Has_Failed then
            raise Program_Error;
        end if;
        Ada.Text_IO.Put_Line ("PASSED");
    end Result;

    function Ident_Str (Value : String) return String is
    begin
        return Value;
    end Ident_Str;

    function Ident_Char (Value : Character) return Character is
    begin
        return Value;
    end Ident_Char;

    function Ident_Wide_Char (Value : Wide_Character) return Wide_Character is
    begin
        return Value;
    end Ident_Wide_Char;

    function Ident_Int (Value : Integer) return Integer is
    begin
        return Value;
    end Ident_Int;

    function Ident_Bool (Value : Boolean) return Boolean is
    begin
        return Value;
    end Ident_Bool;

    function Equal (Left, Right : Integer) return Boolean is
    begin
        return Left = Right;
    end Equal;

    function Legal_File_Name (X : File_Num := 1; Nam : String := "") return String is
        kPrefixes : constant String := "XYZVW";
    begin
        if Nam = "" then
            return kPrefixes (X) & Test_Name (3 .. 7);
        end if;
        return kPrefixes (X) & Nam (3 .. 7);
    end Legal_File_Name;

    procedure Comment (Description : String) is
    begin
        null;
    end Comment;

    procedure Not_Applicable (Description : String) is
    begin
        -- The selected cases are required to be applicable on AIDA's model.
        Failed ("not applicable: " & Description);
    end Not_Applicable;
end Report;
