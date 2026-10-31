procedure PreelaborableErrors is
    type Scalar is new Integer;
    pragma Preelaborable_Initialization (Scalar);
    package Promises is
        type Bad is private;
        pragma Preelaborable_Initialization (Bad);
        protected type Has_Entry is
            entry Wait;
        end Has_Entry;
        pragma Preelaborable_Initialization (Has_Entry);
    private
        type Bad is record
            Value : Integer := Integer'Value ("1");
        end record;
    end Promises;
    package body Promises is
        protected body Has_Entry is
            entry Wait when True is begin null; end Wait;
        end Has_Entry;
    end Promises;
begin
    null;
end PreelaborableErrors;
