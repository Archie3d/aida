with Ada.Finalization;
procedure TaskLegalityErrors is
    task type Worker;
    task body Worker is
    begin
        null;
    end Worker;
    task body Worker is
    begin
        null;
    end Worker;
    subtype AliasWorker is Worker;
    task body AliasWorker is
    begin
        null;
    end AliasWorker;
    object : Worker;
    task body object is
    begin
        null;
    end object;
    type TaggedHolder is new Ada.Finalization.Limited_Controlled with record
        member : Worker;
    end record;
    package BadCompletion is
        type Hidden is private;
    private
        task type Hidden;
    end BadCompletion;
    package body BadCompletion is
        task body Hidden is
        begin
            null;
        end Hidden;
    end BadCompletion;
    package HiddenResult is
        type Hidden is limited private;
        function Make return Hidden;
    private
        task type Hidden;
    end HiddenResult;
    package body HiddenResult is
        task body Hidden is
        begin
            null;
        end Hidden;
        function Make return Hidden is
            object : Hidden;
        begin
            return object;
        end Make;
    end HiddenResult;
begin
    null;
end TaskLegalityErrors;
