with Ada.Unchecked_Deallocation;
procedure GeneralAccessErrors is
    type Link is access all Integer;
    type Read_Only is access constant Integer;
    type Pool is access Integer;
    Value : aliased Integer := 1;
    Plain : Integer := 2;
    Const : aliased constant Integer := 3;
    Positive_Value : aliased Positive := 4;
    P : Link := Plain'Access;
    C : Link := Const'Access;
    M : Link := Positive_Value'Access;
    Q : Pool := Value'Access;
    R : Read_Only := Value'Access;
    Wrong : Link := Link (R);
    procedure Bad_Free is new Ada.Unchecked_Deallocation (Integer, Read_Only);
    function Escape return Link is
        Local : aliased Integer := 5;
    begin
        return Local'Access;
    end Escape;
    type Item (Present : Boolean := True) is record
        case Present is
            when True => Part : aliased Integer;
            when False => null;
        end case;
    end record;
    Object : aliased Item;
    Unstable : Link := Object.Part'Access;
    type Strings is access constant String;
    Text : aliased String := "abc";
    S : Strings := Text'Access;
    type Record_Type is record
        Field : Integer;
    end record;
    type Records is access constant Record_Type;
    Rec : aliased Record_Type := (Field => 1);
    Record_View : Records := Rec'Access;
    procedure Change (X : in out Integer) is
    begin
        X := 0;
    end Change;
begin
    R.all := 7;
    S (1) := 'z';
    S (1 .. 2) := "zz";
    Record_View.Field := 9;
    Change (R.all);
    declare
        Local : aliased Integer;
        type Derived is new Link;
        D : Derived := Local'Access;
        type Inner is access all Integer;
        I : Inner := Local'Access;
    begin
        P := Local'Access;
        P := Link (I);
    end;
    declare
        type Small is record
            Field : Integer;
        end record;
        subtype Padded is Small;
        for Padded'Size use 64;
        Object : aliased Small;
        type Small_Link is access all Small;
        type Large_Link is access all Padded;
        Source : Small_Link := Object'Access;
        Bad_Access : Large_Link := Object'Access;
        Bad_Conversion : Large_Link := Large_Link (Source);
    begin
        null;
    end;
end GeneralAccessErrors;
