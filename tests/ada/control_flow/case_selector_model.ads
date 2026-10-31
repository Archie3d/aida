package Case_Selector_Model is
    type Choice is (First, Middle, Last);
    subtype Window is Integer range 1 .. 3;
    function Select_Value (Value : Integer) return Choice;
    function Select_Value (Value : Integer) return String;
    function With_Default (Value : Integer := 2) return Window;
    function With_Default (Value : Integer := 2) return Float;
end Case_Selector_Model;
