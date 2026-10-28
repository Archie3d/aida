-- Basic definite vectors. Cursor/callback/sorting APIs are a later increment.
with Ada.Finalization;
generic
    type Index_Type is range <>;
    type Element_Type is private;
    with function "=" (Left, Right : Element_Type) return Boolean is <>;
package Ada.Containers.Vectors is
    subtype Extended_Index is Index_Type'Base range Index_Type'First - 1 ..
        Index_Type'Min (Index_Type'Base'Last - 1, Index_Type'Last) + 1;
    No_Index : constant Extended_Index := Extended_Index'First;
    type Vector is tagged private;
    function "=" (Left, Right : Vector) return Boolean;
    function To_Vector (Length : Count_Type) return Vector;
    function To_Vector (New_Item : Element_Type; Length : Count_Type) return Vector;
    function Capacity (Container : Vector) return Count_Type;
    procedure Reserve_Capacity (Container : in out Vector; Capacity : Count_Type);
    function Length (Container : Vector) return Count_Type;
    procedure Set_Length (Container : in out Vector; Length : Count_Type);
    function Is_Empty (Container : Vector) return Boolean;
    procedure Clear (Container : in out Vector);
    function Element (Container : Vector; Index : Index_Type) return Element_Type;
    procedure Replace_Element (Container : in out Vector; Index : Index_Type; New_Item : Element_Type);
    function First_Index (Container : Vector) return Index_Type;
    function Last_Index (Container : Vector) return Extended_Index;
    function First_Element (Container : Vector) return Element_Type;
    function Last_Element (Container : Vector) return Element_Type;
    procedure Append (Container : in out Vector; New_Item : Element_Type; Count : Count_Type := 1);
    procedure Append (Container : in out Vector; New_Item : Vector);
    procedure Prepend (Container : in out Vector; New_Item : Element_Type; Count : Count_Type := 1);
    procedure Insert (Container : in out Vector; Before : Extended_Index;
                      New_Item : Element_Type; Count : Count_Type := 1);
    procedure Delete (Container : in out Vector; Index : Extended_Index; Count : Count_Type := 1);
    procedure Delete_First (Container : in out Vector; Count : Count_Type := 1);
    procedure Delete_Last (Container : in out Vector; Count : Count_Type := 1);
    procedure Swap (Container : in out Vector; I, J : Index_Type);
    procedure Reverse_Elements (Container : in out Vector);
    procedure Move (Target : in out Vector; Source : in out Vector);
    Empty_Vector : constant Vector;
private
    type Element_Access is access Element_Type;
    type Slots is array (Count_Type range <>) of Element_Access;
    type Slots_Access is access Slots;
    type Vector is new Ada.Finalization.Controlled with record
        Data : Slots_Access := null;
        Used : Count_Type := 0;
        Allocated : Count_Type := 0;
    end record;
    overriding procedure Adjust (Object : in out Vector);
    overriding procedure Finalize (Object : in out Vector);
    Empty_Vector : constant Vector :=
        (Ada.Finalization.Controlled with Data => null, Used => 0, Allocated => 0);
end Ada.Containers.Vectors;
