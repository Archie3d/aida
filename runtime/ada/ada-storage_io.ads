with Ada.IO_Exceptions;
with System.Storage_Elements;

generic
    type Element_Type is private;
package Ada.Storage_IO is
    pragma Preelaborate;

    Buffer_Size : constant System.Storage_Elements.Storage_Count :=
        Element_Type'Max_Size_In_Storage_Elements;
    subtype Buffer_Type is
        System.Storage_Elements.Storage_Array (1 .. Buffer_Size);

    procedure Read (Buffer : in Buffer_Type; Item : out Element_Type);
    procedure Write (Buffer : out Buffer_Type; Item : in Element_Type);

    Data_Error : exception renames Ada.IO_Exceptions.Data_Error;
end Ada.Storage_IO;
