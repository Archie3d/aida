-- Ada 2005 container implementation; ownership and cursor machinery is shared.
with Ada.Finalization;
with Ada.Streams;
with Ada.Containers.Storage;
generic
    type Element_Type (<>) is private;
    with function Hash (Element : Element_Type) return Hash_Type;
    with function Equivalent_Elements (Left, Right : Element_Type) return Boolean;
    with function "=" (Left, Right : Element_Type) return Boolean is <>;
package Ada.Containers.Indefinite_Hashed_Sets is
    type Set is tagged private;
    type Cursor is private;
    No_Element : constant Cursor;
    function "=" (Left, Right : Set) return Boolean;
    function Equivalent_Sets (Left, Right : Set) return Boolean;
    function To_Set (New_Item : Element_Type) return Set;
    function Capacity (Container : Set) return Count_Type;
    procedure Reserve_Capacity (Container : in out Set; Capacity : in Count_Type);
    function Length (Container : Set) return Count_Type;
    function Is_Empty (Container : Set) return Boolean;
    procedure Clear (Container : in out Set);
    function Element (Position : Cursor) return Element_Type;
    procedure Replace_Element (Container : in out Set; Position : in Cursor; New_Item : in Element_Type);
    procedure Query_Element (Position : in Cursor; Process : not null access procedure (Element : in
        Element_Type));
    procedure Move (Target : in out Set; Source : in out Set);
    procedure Insert (Container : in out Set; New_Item : in Element_Type; Position : out Cursor; Inserted :
        out Boolean);
    procedure Insert (Container : in out Set; New_Item : in Element_Type);
    procedure Include (Container : in out Set; New_Item : in Element_Type);
    procedure Replace (Container : in out Set; New_Item : in Element_Type);
    procedure Exclude (Container : in out Set; Item : in Element_Type);
    procedure Delete (Container : in out Set; Item : in Element_Type);
    procedure Delete (Container : in out Set; Position : in out Cursor);
    procedure Union (Target : in out Set; Source : in Set);
    function Union (Left, Right : Set) return Set;
    function "or" (Left, Right : Set) return Set renames Union;
    procedure Intersection (Target : in out Set; Source : in Set);
    function Intersection (Left, Right : Set) return Set;
    function "and" (Left, Right : Set) return Set renames Intersection;
    procedure Difference (Target : in out Set; Source : in Set);
    function Difference (Left, Right : Set) return Set;
    function "-" (Left, Right : Set) return Set renames Difference;
    procedure Symmetric_Difference (Target : in out Set; Source : in Set);
    function Symmetric_Difference (Left, Right : Set) return Set;
    function "xor" (Left, Right : Set) return Set renames Symmetric_Difference;
    function Overlap (Left, Right : Set) return Boolean;
    function Is_Subset (Subset : Set; Of_Set : Set) return Boolean;
    function First (Container : Set) return Cursor;
    function Next (Position : Cursor) return Cursor;
    procedure Next (Position : in out Cursor);
    function Find (Container : Set; Item : Element_Type) return Cursor;
    function Contains (Container : Set; Item : Element_Type) return Boolean;
    function Has_Element (Position : Cursor) return Boolean;
    function Equivalent_Elements (Left, Right : Cursor) return Boolean;
    function Equivalent_Elements (Left : Cursor; Right : Element_Type) return Boolean;
    function Equivalent_Elements (Left : Element_Type; Right : Cursor) return Boolean;
    procedure Iterate (Container : in Set; Process : not null access procedure (Position : in Cursor));
    generic
        type Key_Type (<>) is private;
        with function Key (Element : Element_Type) return Key_Type;
        with function Hash (Key : Key_Type) return Hash_Type;
        with function Equivalent_Keys (Left, Right : Key_Type) return Boolean;
    package Generic_Keys is
        function Key (Position : Cursor) return Key_Type;
        function Element (Container : Set; Key : Key_Type) return Element_Type;
        procedure Replace (Container : in out Set; Key : in Key_Type; New_Item : in Element_Type);
        procedure Exclude (Container : in out Set; Key : in Key_Type);
        procedure Delete (Container : in out Set; Key : in Key_Type);
        function Find (Container : Set; Key : Key_Type) return Cursor;
        function Contains (Container : Set; Key : Key_Type) return Boolean;
        procedure Update_Element_Preserving_Key (Container : in out Set; Position : in Cursor; Process : not
            null access procedure (Element : in out Element_Type));
    end Generic_Keys;
    Empty_Set : constant Set;
private
    subtype Key_Box is Natural;
    subtype Element_Box is Element_Type;
    function Box_Key (Value : Natural) return Key_Box;
    function Box_Element (Value : Element_Type) return Element_Box;
    package Storage is new Ada.Containers.Storage
        (Natural, Element_Type, Key_Box, Element_Box, Box_Key, Box_Element, False, True, False);
    type Cursor is record
        Ptr : Storage.Position := null;
    end record;
    type Set is tagged record
        Data : Storage.Store;
    end record;
    procedure Read_Container (Stream : access Ada.Streams.Root_Stream_Type'Class;
                              Item : out Set);
    procedure Write_Container (Stream : access Ada.Streams.Root_Stream_Type'Class;
                               Item : Set);
    for Set'Read use Read_Container;
    for Set'Write use Write_Container;
    No_Element : constant Cursor := (Ptr => null);
    Empty_Set : constant Set :=
        (Data => (Ada.Finalization.Controlled with Data => null));
end Ada.Containers.Indefinite_Hashed_Sets;
