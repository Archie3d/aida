package body Case_Selector_Model is
    function Select_Value (Value : Integer) return Choice is
    begin
        return Choice'Val (Value);
    end Select_Value;
    function Select_Value (Value : Integer) return String is
    begin
        raise Program_Error;
    end Select_Value;
    function With_Default (Value : Integer := 2) return Window is
    begin
        return Value;
    end With_Default;
    function With_Default (Value : Integer := 2) return Float is
    begin
        raise Program_Error;
    end With_Default;
end Case_Selector_Model;
