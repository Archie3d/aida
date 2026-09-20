with Ada.Text_IO; use Ada.Text_IO;
procedure NestedOverloads is
    Calls : Integer := 0;
    function Pick return Integer is
    begin
        Calls := Calls + 1;
        return 7;
    end Pick;
    function Pick return Boolean is
    begin
        Calls := Calls + 100;
        return False;
    end Pick;
    procedure Consume (Value : Boolean; Flag : Integer) is
    begin
        raise Program_Error;
    end Consume;
    procedure Consume (Value : Integer; Flag : Boolean) is
    begin
        Put_Line (Integer'Image (Value));
    end Consume;
    function Wrap (Value : Boolean) return Boolean is
    begin
        return Value;
    end Wrap;
    function Wrap (Value : Integer) return Integer is
    begin
        return Value + 1;
    end Wrap;
    function Result (Value : Boolean) return Boolean is
    begin
        return Value;
    end Result;
    function Result (Value : Integer) return Integer is
    begin
        return Value;
    end Result;
    procedure Defaults (Value : Boolean; Flag : Integer := 0) is
    begin
        raise Program_Error;
    end Defaults;
    procedure Defaults (Value : Integer; Flag : Boolean := True) is
    begin
        Put_Line (Integer'Image (Value));
    end Defaults;
    package Names is
        function Get (Value : Integer := 9) return Integer;
        function Get (Value : Integer := 9) return Boolean;
        type First is (One, Two);
        type Second is (One, Two);
    end Names;
    package body Names is
        function Get (Value : Integer := 9) return Integer is
        begin
            return Value;
        end Get;
        function Get (Value : Integer := 9) return Boolean is
        begin
            return False;
        end Get;
    end Names;
    procedure Enum (Value : Names.First; Flag : Boolean) is
    begin
        Put_Line (Names.First'Image (Value));
    end Enum;
    procedure Enum (Value : Names.Second; Flag : Integer) is
    begin
        raise Program_Error;
    end Enum;
    type First_Record is record
        Value : Integer;
    end record;
    type Second_Record is record
        Value : Boolean;
    end record;
    function Record_Pick return First_Record is
    begin
        return (Value => 12);
    end Record_Pick;
    function Record_Pick return Second_Record is
    begin
        return (Value => False);
    end Record_Pick;
    function Default_Pick (Value : Integer := 15) return Integer is
    begin
        return Value;
    end Default_Pick;
    function Default_Pick (Value : Integer) return Boolean is
    begin
        return False;
    end Default_Pick;
    procedure Only_Default (Value : Integer; Extra : Boolean := True) is
    begin
        Put_Line (Integer'Image (Value));
    end Only_Default;
    procedure Only_Default (Value : Boolean; Required : Integer) is
    begin
        raise Program_Error;
    end Only_Default;
    function Same_Result (Value : Integer) return Integer is
    begin
        return Value;
    end Same_Result;
    function Same_Result (Value : Boolean) return Integer is
    begin
        raise Program_Error;
        return 0;
    end Same_Result;
    type Vector is array (1 .. 7) of Integer;
    Values : Vector := (others => 21);
    X : Integer;
begin
    Consume (Pick, True);
    -- Repeated candidate discovery must not grow exponentially with depth.
    Consume (((((((((((((((((((((1 + 1) + 1) + 1) + 1) + 1) + 1) + 1) + 1) + 1) + 1) + 1) + 1) + 1) + 1) + 1) + 1) + 1) + 1) + 1) + 1), True);
    Consume (Wrap (Wrap (Pick)), True);
    Consume (Flag => True, Value => Pick);
    Defaults (Pick, Flag => True);
    Consume (Names.Get, True);
    Consume (Names.Get (3), True);
    Enum (Names.One, True);
    X := Result (Wrap (Pick));
    Put_Line (Integer'Image (X));
    Consume (-(Wrap (Pick) + 1), True);
    if Pick = 7 then
        Put_Line ("comparison");
    end if;
    Put_Line (Integer'Image (Calls));
    Consume (Record_Pick.Value, True);
    Consume (Values (Pick), True);
    Only_Default (Pick);
    X := Default_Pick;
    Put_Line (Integer'Image (X));
    X := Same_Result (Wrap (2));
    Put_Line (Integer'Image (X));
    if not Pick then
        Put_Line ("not");
    end if;
    declare
        function "**" (Left : Boolean; Right : Integer) return Integer is
        begin
            return Right + 10;
        end "**";
    begin
        X := True ** Pick;
        Put_Line (Integer'Image (X));
    end;
end NestedOverloads;
