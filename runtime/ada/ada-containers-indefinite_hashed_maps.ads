-- Ada 2005 container implementation; ownership and cursor machinery is shared.
with Ada.Finalization;
with Ada.Containers.Storage;
generic
    type Key_Type (<>) is private;
    type Element_Type (<>) is private;
    with function Hash (Key : Key_Type) return Hash_Type;
    with function Equivalent_Keys (Left, Right : Key_Type) return Boolean;
    with function "=" (Left, Right : Element_Type) return Boolean is <>;
package Ada.Containers.Indefinite_Hashed_Maps is
    type Map is tagged private;
    type Cursor is private;
    No_Element : constant Cursor;
    function "=" (Left, Right : Map) return Boolean;
    function Capacity (Container : Map) return Count_Type;
    procedure Reserve_Capacity (Container : in out Map; Capacity : in Count_Type);
    function Length (Container : Map) return Count_Type;
    function Is_Empty (Container : Map) return Boolean;
    procedure Clear (Container : in out Map);
    function Key (Position : Cursor) return Key_Type;
    function Element (Position : Cursor) return Element_Type;
    procedure Replace_Element (Container : in out Map; Position : in Cursor; New_Item : in Element_Type);
    procedure Query_Element (Position : in Cursor; Process : not null access procedure (Key : in Key_Type;
        Element : in Element_Type));
    procedure Update_Element (Container : in out Map; Position : in Cursor; Process : not null access
        procedure (Key : in Key_Type; Element : in out Element_Type));
    procedure Move (Target : in out Map; Source : in out Map);
    procedure Insert (Container : in out Map; Key : in Key_Type; New_Item : in Element_Type; Position : out
        Cursor; Inserted : out Boolean);
    procedure Insert (Container : in out Map; Key : in Key_Type; New_Item : in Element_Type);
    procedure Include (Container : in out Map; Key : in Key_Type; New_Item : in Element_Type);
    procedure Replace (Container : in out Map; Key : in Key_Type; New_Item : in Element_Type);
    procedure Exclude (Container : in out Map; Key : in Key_Type);
    procedure Delete (Container : in out Map; Key : in Key_Type);
    procedure Delete (Container : in out Map; Position : in out Cursor);
    function First (Container : Map) return Cursor;
    function Next (Position : Cursor) return Cursor;
    procedure Next (Position : in out Cursor);
    function Find (Container : Map; Key : Key_Type) return Cursor;
    function Element (Container : Map; Key : Key_Type) return Element_Type;
    function Contains (Container : Map; Key : Key_Type) return Boolean;
    function Has_Element (Position : Cursor) return Boolean;
    function Equivalent_Keys (Left, Right : Cursor) return Boolean;
    function Equivalent_Keys (Left : Cursor; Right : Key_Type) return Boolean;
    function Equivalent_Keys (Left : Key_Type; Right : Cursor) return Boolean;
    procedure Iterate (Container : in Map; Process : not null access procedure (Position : in Cursor));
    Empty_Map : constant Map;
private
    subtype Key_Box is Key_Type;
    subtype Element_Box is Element_Type;
    function Box_Key (Value : Key_Type) return Key_Box;
    function Box_Element (Value : Element_Type) return Element_Box;
    package Storage is new Ada.Containers.Storage
        (Key_Type, Element_Type, Key_Box, Element_Box, Box_Key, Box_Element, False, True, False);
    type Cursor is record
        Ptr : Storage.Position := null;
    end record;
    type Map is tagged record
        Data : Storage.Store;
    end record;
    No_Element : constant Cursor := (Ptr => null);
    Empty_Map : constant Map :=
        (Data => (Ada.Finalization.Controlled with Data => null));
end Ada.Containers.Indefinite_Hashed_Maps;
