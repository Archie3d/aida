-- Ada 2005 container implementation; ownership and cursor machinery is shared.
with Ada.Finalization;
with Ada.Containers.Storage;
generic
    type Element_Type (<>) is private;
    with function "=" (Left, Right : Element_Type) return Boolean is <>;
package Ada.Containers.Indefinite_Doubly_Linked_Lists is
    type List is tagged private;
    type Cursor is private;
    No_Element : constant Cursor;
    function "=" (Left, Right : List) return Boolean;
    function Length (Container : List) return Count_Type;
    function Is_Empty (Container : List) return Boolean;
    procedure Clear (Container : in out List);
    function Element (Position : Cursor) return Element_Type;
    procedure Replace_Element (Container : in out List; Position : in Cursor; New_Item : in Element_Type);
    procedure Query_Element (Position : in Cursor; Process : not null access procedure (Element : in
        Element_Type));
    procedure Update_Element (Container : in out List; Position : in Cursor; Process : not null access
        procedure (Element : in out Element_Type));
    procedure Move (Target : in out List; Source : in out List);
    procedure Insert (Container : in out List; Before : in Cursor; New_Item : in Element_Type; Count : in
        Count_Type := 1);
    procedure Insert (Container : in out List; Before : in Cursor; New_Item : in Element_Type; Position : out
        Cursor; Count : in Count_Type := 1);
    procedure Prepend (Container : in out List; New_Item : in Element_Type; Count : in Count_Type := 1);
    procedure Append (Container : in out List; New_Item : in Element_Type; Count : in Count_Type := 1);
    procedure Delete (Container : in out List; Position : in out Cursor; Count : in Count_Type := 1);
    procedure Delete_First (Container : in out List; Count : in Count_Type := 1);
    procedure Delete_Last (Container : in out List; Count : in Count_Type := 1);
    procedure Reverse_Elements (Container : in out List);
    procedure Swap (Container : in out List; I, J : in Cursor);
    procedure Swap_Links (Container : in out List; I, J : in Cursor);
    procedure Splice (Target : in out List; Before : in Cursor; Source : in out List);
    procedure Splice (Target : in out List; Before : in Cursor; Source : in out List; Position : in out
        Cursor);
    procedure Splice (Container: in out List; Before : in Cursor; Position : in Cursor);
    function First (Container : List) return Cursor;
    function First_Element (Container : List) return Element_Type;
    function Last (Container : List) return Cursor;
    function Last_Element (Container : List) return Element_Type;
    function Next (Position : Cursor) return Cursor;
    function Previous (Position : Cursor) return Cursor;
    procedure Next (Position : in out Cursor);
    procedure Previous (Position : in out Cursor);
    function Find (Container : List; Item : Element_Type; Position : Cursor := No_Element) return Cursor;
    function Reverse_Find (Container : List; Item : Element_Type; Position : Cursor := No_Element) return
        Cursor;
    function Contains (Container : List; Item : Element_Type) return Boolean;
    function Has_Element (Position : Cursor) return Boolean;
    procedure Iterate (Container : in List; Process : not null access procedure (Position : in Cursor));
    procedure Reverse_Iterate (Container : in List; Process : not null access procedure (Position : in
        Cursor));
    generic
        with function "<" (Left, Right : Element_Type) return Boolean is <>;
    package Generic_Sorting is
        function Is_Sorted (Container : List) return Boolean;
        procedure Sort (Container : in out List);
        procedure Merge (Target : in out List; Source : in out List);
    end Generic_Sorting;
    Empty_List : constant List;
private
    subtype Key_Box is Natural;
    subtype Element_Box is Element_Type;
    function Box_Key (Value : Natural) return Key_Box;
    function Box_Element (Value : Element_Type) return Element_Box;
    package Storage is new Ada.Containers.Storage
        (Natural, Element_Type, Key_Box, Element_Box, Box_Key, Box_Element, False);
    type Cursor is record
        Ptr : Storage.Position := null;
    end record;
    type List is tagged record
        Data : Storage.Store;
    end record;
    No_Element : constant Cursor := (Ptr => null);
    Empty_List : constant List :=
        (Data => (Ada.Finalization.Controlled with Data => null));
end Ada.Containers.Indefinite_Doubly_Linked_Lists;
