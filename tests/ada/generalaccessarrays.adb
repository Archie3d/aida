with Ada.Text_IO; use Ada.Text_IO;
with Ada.Unchecked_Deallocation;
procedure GeneralAccessArrays is
    type String_Link is access all String;
    type String_Constant is access constant String;
    Data : aliased String := "abc";
    Alias : String renames Data;
    P : String_Link := Alias'Access;
    C : String_Constant := Data'Access;
    type Matrix is array (Positive range <>, Positive range <>) of Integer;
    type Matrix_Link is access all Matrix;
    Grid : aliased Matrix := ((1, 2), (3, 4));
    G : Matrix_Link := Grid'Access;
    type Holder is record
        Text : aliased String (1 .. 3) := "xyz";
    end record;
    type Holders is array (Positive range <>) of Holder;
    type Holders_Link is access all Holders;
    Overlaid : aliased Holders := (1 .. 2 => (Text => "xyz"));
    type Text_Link is access all String (1 .. 3);
    Whole : Holders_Link := Overlaid'Access;
    Part : Text_Link := Overlaid (1).Text'Access;
    procedure Free is new Ada.Unchecked_Deallocation (String, String_Link);
    procedure Check (Condition : Boolean) is
    begin
        if not Condition then
            raise Program_Error;
        end if;
    end Check;
begin
    Check (P.all = "abc" and P'Length = 3 and P'First = 1);
    P (2) := 'z';
    Check (Data = "azc" and C.all = "azc" and P = Data'Access);
    Check (C (1 .. 2) = "az");
    Check (Whole'Length = 2 and Part'Length = 3 and Part.all = "xyz");
    Part (2) := 'q';
    Check (Whole (1).Text = "xqz");
    G (2, 1) := 9;
    Check (Grid (2, 1) = 9 and G'Length (2) = 2);
    declare
        type Inner_Link is access all String;
        Inner : Inner_Link := Data'Access;
    begin
        Check (Inner.all = "azc");
    end;
    Check (P.all = "azc");
    declare
        type Inner_Link is access all String;
        Inner : Inner_Link := Data'Access;
        Other : String_Link := Data'Access;
    begin
        Check (Inner.all = Other.all);
    end;
    Check (P.all = "azc");
    P := new String'("heap");
    Check (P.all = "heap" and P'Length = 4);
    Free (P);
    Check (P = null and Data = "azc");
    Put_Line ("borrowed array access passed");
end GeneralAccessArrays;
