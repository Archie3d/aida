-- Deliberately malformed replacement of the intrinsic package interface.
generic
    type Object (<>) is limited private;
package System.Address_To_Access_Conversions is
    pragma Preelaborate;
    type Object_Pointer is access all Object;
    function To_Pointer return Object_Pointer;
    pragma Import (C, To_Pointer, "__ada_address_to_pointer");
    function To_Address (Value : Object_Pointer) return Address;
    pragma Import (C, To_Address, "__ada_pointer_to_address");
end System.Address_To_Access_Conversions;
