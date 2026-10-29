-- Ada 2005 container implementation; ownership and cursor machinery is shared.
with Ada.Finalization;
with Ada.Streams;
with Ada.Containers.Storage;
generic
    type Index_Type is range <>;
    type Element_Type is private;
    with function "=" (Left, Right : Element_Type) return Boolean is <>;
package Ada.Containers.Vectors is
    subtype Extended_Index is
        Index_Type'Base range
            Index_Type'First-1 ..
            Index_Type'Min (Index_Type'Base'Last - 1, Index_Type'Last) + 1;
    No_Index : constant Extended_Index := Extended_Index'First;
    type Vector is tagged private;
    type Cursor is private;
    No_Element : constant Cursor;
    function "=" (Left, Right : Vector) return Boolean;
    function To_Vector (Length : Count_Type) return Vector;
    function To_Vector (New_Item : Element_Type; Length : Count_Type) return Vector;
    function "&" (Left, Right : Vector) return Vector;
    function "&" (Left : Vector; Right : Element_Type) return Vector;
    function "&" (Left : Element_Type; Right : Vector) return Vector;
    function "&" (Left, Right : Element_Type) return Vector;
    function Capacity (Container : Vector) return Count_Type;
    procedure Reserve_Capacity (Container : in out Vector; Capacity : in Count_Type);
    function Length (Container : Vector) return Count_Type;
    procedure Set_Length (Container : in out Vector; Length : in Count_Type);
    function Is_Empty (Container : Vector) return Boolean;
    procedure Clear (Container : in out Vector);
    function To_Cursor (Container : Vector; Index : Extended_Index) return Cursor;
    function To_Index (Position : Cursor) return Extended_Index;
    function Element (Container : Vector; Index : Index_Type) return Element_Type;
    function Element (Position : Cursor) return Element_Type;
    procedure Replace_Element (Container : in out Vector; Index : in Index_Type; New_Item : in Element_Type);
    procedure Replace_Element (Container : in out Vector; Position : in Cursor; New_item : in Element_Type);
    procedure Query_Element (Container : in Vector; Index : in Index_Type; Process : not null access procedure
        (Element : in Element_Type));
    procedure Query_Element (Position : in Cursor; Process : not null access procedure (Element : in
        Element_Type));
    procedure Update_Element (Container : in out Vector; Index : in Index_Type; Process : not null access
        procedure (Element : in out Element_Type));
    procedure Update_Element (Container : in out Vector; Position : in Cursor; Process : not null access
        procedure (Element : in out Element_Type));
    procedure Move (Target : in out Vector; Source : in out Vector);
    procedure Insert (Container : in out Vector; Before : in Extended_Index; New_Item : in Vector);
    procedure Insert (Container : in out Vector; Before : in Cursor; New_Item : in Vector);
    procedure Insert (Container : in out Vector; Before : in Cursor; New_Item : in Vector; Position : out
        Cursor);
    procedure Insert (Container : in out Vector; Before : in Extended_Index; New_Item : in Element_Type; Count
        : in Count_Type := 1);
    procedure Insert (Container : in out Vector; Before : in Cursor; New_Item : in Element_Type; Count : in
        Count_Type := 1);
    procedure Insert (Container : in out Vector; Before : in Cursor; New_Item : in Element_Type; Position :
        out Cursor; Count : in Count_Type := 1);
    procedure Insert (Container : in out Vector; Before : in Extended_Index; Count : in Count_Type := 1);
    procedure Insert (Container : in out Vector; Before : in Cursor; Position : out Cursor; Count : in
        Count_Type := 1);
    procedure Prepend (Container : in out Vector; New_Item : in Vector);
    procedure Prepend (Container : in out Vector; New_Item : in Element_Type; Count : in Count_Type := 1);
    procedure Append (Container : in out Vector; New_Item : in Vector);
    procedure Append (Container : in out Vector; New_Item : in Element_Type; Count : in Count_Type := 1);
    procedure Insert_Space (Container : in out Vector; Before : in Extended_Index; Count : in Count_Type :=
        1);
    procedure Insert_Space (Container : in out Vector; Before : in Cursor; Position : out Cursor; Count : in
        Count_Type := 1);
    procedure Delete (Container : in out Vector; Index : in Extended_Index; Count : in Count_Type := 1);
    procedure Delete (Container : in out Vector; Position : in out Cursor; Count : in Count_Type := 1);
    procedure Delete_First (Container : in out Vector; Count : in Count_Type := 1);
    procedure Delete_Last (Container : in out Vector; Count : in Count_Type := 1);
    procedure Reverse_Elements (Container : in out Vector);
    procedure Swap (Container : in out Vector; I, J : in Index_Type);
    procedure Swap (Container : in out Vector; I, J : in Cursor);
    function First_Index (Container : Vector) return Index_Type;
    function First (Container : Vector) return Cursor;
    function First_Element (Container : Vector) return Element_Type;
    function Last_Index (Container : Vector) return Extended_Index;
    function Last (Container : Vector) return Cursor;
    function Last_Element (Container : Vector) return Element_Type;
    function Next (Position : Cursor) return Cursor;
    procedure Next (Position : in out Cursor);
    function Previous (Position : Cursor) return Cursor;
    procedure Previous (Position : in out Cursor);
    function Find_Index (Container : Vector; Item : Element_Type; Index : Index_Type := Index_Type'First)
        return Extended_Index;
    function Find (Container : Vector; Item : Element_Type; Position : Cursor := No_Element) return Cursor;
    function Reverse_Find_Index (Container : Vector; Item : Element_Type; Index : Index_Type :=
        Index_Type'Last) return Extended_Index;
    function Reverse_Find (Container : Vector; Item : Element_Type; Position : Cursor := No_Element) return
        Cursor;
    function Contains (Container : Vector; Item : Element_Type) return Boolean;
    function Has_Element (Position : Cursor) return Boolean;
    procedure Iterate (Container : in Vector; Process : not null access procedure (Position : in Cursor));
    procedure Reverse_Iterate (Container : in Vector; Process : not null access procedure (Position : in
        Cursor));
    generic
        with function "<" (Left, Right : Element_Type) return Boolean is <>;
    package Generic_Sorting is
        function Is_Sorted (Container : Vector) return Boolean;
        procedure Sort (Container : in out Vector);
        procedure Merge (Target : in out Vector; Source : in out Vector);
    end Generic_Sorting;
    Empty_Vector : constant Vector;
private
    subtype Key_Box is Natural;
    type Element_Box is record
        Value : Element_Type;
    end record;
    function Box_Key (Value : Natural) return Key_Box;
    function Box_Element (Value : Element_Type) return Element_Box;
    package Storage is new Ada.Containers.Storage
        (Natural, Element_Type, Key_Box, Element_Box, Box_Key, Box_Element, True);
    type Cursor is record
        Ptr : Storage.Position := null;
    end record;
    type Vector is tagged record
        Data : Storage.Store;
    end record;
    procedure Read_Container (Stream : access Ada.Streams.Root_Stream_Type'Class;
                              Item : out Vector);
    procedure Write_Container (Stream : access Ada.Streams.Root_Stream_Type'Class;
                               Item : Vector);
    for Vector'Read use Read_Container;
    for Vector'Write use Write_Container;
    No_Element : constant Cursor := (Ptr => null);
    Empty_Vector : constant Vector :=
        (Data => (Ada.Finalization.Controlled with Data => null));
end Ada.Containers.Vectors;
