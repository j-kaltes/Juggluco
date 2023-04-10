package tk.glucodata.NovoPen.opennov;

/**
 * JamOrHam
 * OpenNov state type
 */
public class FSA {
    public final Action action;
    public final byte[] payload;


    public enum Action {
        WRITE, WRITE_READ, READ, LOOP, DONE;
    }

    public static FSA empty() {
        return new FSA(Action.DONE, null);
    }

    public static FSA loop() {
        return new FSA(Action.LOOP, null);
    }

    public static FSA read() {
        return new FSA(Action.READ, null);
    }

    public static FSA writeRead(final byte[] payload) {
        return new FSA(Action.WRITE_READ, payload);
    }

    public static FSA writeNull() {
        return writeRead(new byte[0]);
    }

    public boolean doRead() {
        return action == Action.READ || action == Action.WRITE_READ;
    }

    @Override
    public String toString() {
        return "FSA(action=" + this.getAction() + ", payload=" + java.util.Arrays.toString(this.getPayload()) + ")";
    }

    public FSA(final Action action, final byte[] payload) {
        this.action = action;
        this.payload = payload;
    }

    public Action getAction() {
        return this.action;
    }

    public byte[] getPayload() {
        return this.payload;
    }

    @Override
    public boolean equals(final Object o) {
        if (o == this) return true;
        if (!(o instanceof FSA)) return false;
        final FSA other = (FSA) o;
        if (!other.canEqual((Object) this)) return false;
        final Object this$action = this.getAction();
        final Object other$action = other.getAction();
        if (this$action == null ? other$action != null : !this$action.equals(other$action)) return false;
        if (!java.util.Arrays.equals(this.getPayload(), other.getPayload())) return false;
        return true;
    }

    protected boolean canEqual(final Object other) {
        return other instanceof FSA;
    }

    @Override
    public int hashCode() {
        final int PRIME = 59;
        int result = 1;
        final Object $action = this.getAction();
        result = result * PRIME + ($action == null ? 43 : $action.hashCode());
        result = result * PRIME + java.util.Arrays.hashCode(this.getPayload());
        return result;
    }
}
