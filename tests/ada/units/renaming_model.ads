package Renaming_Model is
    Value : Integer := 1;
    procedure Increase;
    package Nested is
        Value : Integer := 3;
    end Nested;
    generic
    function Factory return Integer;
end Renaming_Model;
