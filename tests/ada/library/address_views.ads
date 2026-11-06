with System.Address_To_Access_Conversions;
package Address_Views is
    package Integers is new System.Address_To_Access_Conversions (Integer);
    type Hidden is private;
    function Create (Value : Integer) return Hidden;
    function Value (Item : Hidden) return Integer;
private
    type Hidden is record
        Data : Integer;
    end record;
end Address_Views;
