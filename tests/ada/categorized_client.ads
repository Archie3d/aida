with Categorized_Types;
with Categorized_Generic;
with Categorized_Pure;
with Categorized_Math;
package Categorized_Client is
    pragma Preelaborate;
    package Sum is new Categorized_Math ("+");
    package Items is new Categorized_Generic (Categorized_Types.Value);
    Guard : Categorized_Types.Lock;
    type Letters is array (1 .. 4) of Character;
    Text : Letters := (others => 'x');
    Length : constant Integer := Text'Length;
    type Pointer is access Integer;
    Data : Pointer := new Integer'(23);
    Operation : constant Categorized_Pure.Callback := Categorized_Pure.Increment'Access;
end Categorized_Client;
