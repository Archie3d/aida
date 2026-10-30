-- Internal file-handle ABI. The compiler registers this scalar component with
-- the ordinary finalization chain. Only the address that opened a file owns it;
-- Text_IO standard/current results borrow it and never close their owner.
with System;
package Ada.IO_Handles is
    type File_Handle is new System.Address;
end Ada.IO_Handles;
