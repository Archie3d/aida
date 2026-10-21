with Ada.Text_IO;

package body Report is
    Has_Failed : Boolean := False;

    procedure Test (Name : String; Description : String) is
    begin
        Has_Failed := False;
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
