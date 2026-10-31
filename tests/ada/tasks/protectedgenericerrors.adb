generic
package Protected_Template is
    protected type Counter is
        procedure Increment;
    end Counter;
end Protected_Template;

with Protected_Template;
package Protected_Instance is new Protected_Template;

with Protected_Instance;
procedure ProtectedGenericErrors is
begin
    null;
end ProtectedGenericErrors;
