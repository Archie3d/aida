limited private with Context_Impl.Child;
package Context_Impl is
    type Handle is private;
    function Create return Handle;
    function Read (Object : Handle) return Integer;
private
    type Pointer is access Context_Impl.Child.Data;
    type Handle is record
        Object : Pointer;
    end record;
end Context_Impl;
