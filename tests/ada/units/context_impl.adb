with Context_Impl.Child;
package body Context_Impl is
    function Create return Handle is
        Object : Handle;
    begin
        Object.Object := new Context_Impl.Child.Data;
        return Object;
    end Create;
    function Read (Object : Handle) return Integer is
    begin
        return Object.Object.Value;
    end Read;
end Context_Impl;
