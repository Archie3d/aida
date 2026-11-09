-- 64-bit byte-addressed target model; arithmetic does not designate storage.
package System.Storage_Elements is
    pragma Pure;

    type Storage_Offset is range System.Min_Int .. System.Max_Int;
    subtype Storage_Count is Storage_Offset range 0 .. Storage_Offset'Last;

    type Storage_Element is mod 2 ** System.Storage_Unit;
    for Storage_Element'Size use System.Storage_Unit;
    type Storage_Array is array (Storage_Offset range <>) of aliased Storage_Element;
    for Storage_Array'Component_Size use System.Storage_Unit;

    function "+" (Left : Address; Right : Storage_Offset) return Address;
    pragma Import (C, "+", "__ada_storage_add");
    function "+" (Left : Storage_Offset; Right : Address) return Address;
    pragma Import (C, "+", "__ada_storage_offset_add");
    function "-" (Left : Address; Right : Storage_Offset) return Address;
    pragma Import (C, "-", "__ada_storage_subtract");
    function "-" (Left, Right : Address) return Storage_Offset;
    pragma Import (C, "-", "__ada_storage_difference");
    function "mod" (Left : Address; Right : Storage_Offset) return Storage_Offset;
    pragma Import (C, "mod", "__ada_storage_mod");

    type Integer_Address is mod System.Memory_Size;
    function To_Address (Value : Integer_Address) return Address;
    pragma Import (C, To_Address, "__ada_storage_to_address");
    function To_Integer (Value : Address) return Integer_Address;
    pragma Import (C, To_Integer, "__ada_storage_to_integer");
end System.Storage_Elements;
