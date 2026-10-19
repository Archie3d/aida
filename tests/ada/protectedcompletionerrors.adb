procedure ProtectedCompletionErrors is
    package Model is
        type Hidden is limited private;
        type Holder is record
            item : Hidden;
            suffix : Integer;
        end record;
        type Values is array (1 .. 2) of Hidden;
        first : Hidden;
        copy : Hidden := first;
    private
        protected type Hidden is
            function Get return Integer;
        end Hidden;
    end Model;
    package body Model is
        protected body Hidden is
            function Get return Integer is
            begin
                return 0;
            end Get;
        end Hidden;
    end Model;
begin
    null;
end ProtectedCompletionErrors;
