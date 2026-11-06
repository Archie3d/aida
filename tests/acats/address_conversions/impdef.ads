-- Address representation formatting adapter for the unchanged FD72A00 support.
with Ada.Text_IO;
with System.Storage_Elements;
package Impdef is
    package Address_Value_IO is new Ada.Text_IO.Modular_IO
        (System.Storage_Elements.Integer_Address);
end Impdef;
