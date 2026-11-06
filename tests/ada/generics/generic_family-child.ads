with Child_Context;
generic
    Step : Integer := Child_Context.Default_Step;
package Generic_Family.Child is
    procedure Update (Value : in out Cell);
    procedure Fail;
    function Parent_Seed return Element;
    function Calls return Integer;
private
    Local_Count : Integer := 0;
end Generic_Family.Child;
