package Derived_Profile_Model is
    Calls : Integer := 0;
    type Item (Round : Boolean) is record
        case Round is
            when True => Radius : Integer := 7;
            when False => Width : Integer := 9;
        end case;
    end record;
    subtype Circle is Item (True);
    subtype Alias_Circle is Circle;
    function Make (Round : Boolean) return Item;
    function Read (Object : Alias_Circle) return Integer;
    function Inspect (Object : Item) return Boolean;
    procedure Touch (Object : in out Alias_Circle);
    procedure Reset (Object : out Alias_Circle);
end Derived_Profile_Model;
