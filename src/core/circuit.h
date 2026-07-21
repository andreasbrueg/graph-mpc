#pragma once

#include <memory>
#include <optional>

#include "../utils/structs.h"
#include "gate.h"

/**
 * @brief Interface for representing values for node states, messages, etc.
 * 
 * mp_val logically represents one scalar value being a node state, passed message, update, or
 * similar. Technically, it represents the ID of the wire in the circuit that holds the
 * corresponding value, and the wire is a SIMD-wire, i.e., it represents a value for each DAG list
 * entry. As operations are carried out in a SIMD-fashion over all DAG list entries, mp_val can
 * usually be used as if it was a simple scalar value.
 * 
 * Note that to operate on mp_val, the Circuit class provides custom operations which need to be
 * used. Instead using, e.g., native +, -, * etc. operations would run operations on wire IDs, i.e.,
 * addresses which is not intended.
 * 
 * This typedef simply is used to make the high-level interface cleaner while abstracting away the
 * actual semantics of the SIMD wires.
 */
typedef SIMD_wire_id mp_val;

/**
 * @brief Circuit representing an algorithm to be computed.
 * 
 * The circuit class is used to define arithmetic circuits which are to be evaluated in MPC.
 * It practically represents an algorithm with the restriction that the entire program flow is
 * static, i.e., the program flow is data-oblivious. This is needed as no information about the
 * private input data can be leaked.
 * 
 * This class acts as the superclass for any specific graph algorithm to be implemented. Any graph
 * algorithm shall be placed in the algorithms directory and be a subclass of Circuit. There are
 * multiple abstract methods of Circuit that need to be specified by the graph algorithm subclass,
 * and there are other methods with some default implementation which can be overridden when needed.
 * 
 * Furthermore, this class provides a library of operations that can be used to implement the
 * (abstract) methods for graph analysis.
 */
class Circuit {

  public:

    /**
     * @brief Construct a new Circuit object
     * 
     * As an option, a number of columns >1 can be specified to use multiple data columns. This means
     * that each node's state contains 'columns' many scalar values, and 'columns' many instances of
     * message-passing will run in parallel, one on each column.
     * 
     * @param conf ProtocolConfig for which the circuit is created
     * @param columns optional, can be set to a value >1 to use multiple data columns
     */
    Circuit(ProtocolConfig &conf, size_t columns = 1)
        :   size(conf.size),
            nodes(conf.nodes),
            bits(conf.bits),
            depth(conf.depth),
            n_shuffles(0),
            n_mults(0),
            n_wires(0),
            shuffle_idx(0),
            columns(columns) {
                if (columns > 1) in.data_parallel.resize(columns);
            }

    /**
     * @brief Method building the circuit, finalizing its specification
     * 
     * This method needs to be called once all parameters are passed to the circuit and potential
     * settings have been made. After it has been called, the computed algorithm is fixed and can
     * no longer be modified.
     */
    void build();

    /**
     * @brief Force outputs to be provided in the same DAG list order as the inputs.
     *          Call before build()!
     * 
     * Normally, PPGA outputs the node states sorted by node id. If this method is called before
     * build(), PPGA instead outputs the node states in the order of the DAG list as provided in
     * the input, interleaved with zeros for all edges.
     * 
     * This is useful if outputs are returned to clients who provided the graph inputs, to allow
     * that each client gets the data for exactly the nodes that it also provided as input.
     */
    void provide_outputs_in_input_order();

    /**
     * @brief Switches message-passing to traverse edges backwards. Call before build()!
     * 
     * PPGA by default propagates messages from nodes to their outgoing edges, and gathers them for
     * nodes from their incoming edges. Calling this method before build() reverses this behavior,
     * so that messages are propagated by nodes to their incoming edges and updates are gathered
     * for nodes from their outgoing edges.
     * 
     * This is useful if for some reason, data needs to be sent via the edges "backwards".
     * 
     * If this is specific to an algorithm, it is recommended to call the function in the
     * corresponding algorithm's constructor.
     */
    void use_reverse_message_passing();

    /**
     * @brief Removes any duplicate edges. Call before build()!
     * 
     * PPGA supports multigraphs where multiple edges from the same node v to the same node w can
     * exist. In this case, a message m is sent accross both edges, arriving at w twice, hence,
     * contributing 2*m to its update. In cases where multigraphs may be used as inputs, but the
     * algorithm shall run on a standard simple graph, this method can be called before build() to
     * remove all duplicate edges, leaving only one single edge from v to w.
     * 
     * Technically, duplicate edges are not removed, as this would leak the number of duplicates.
     * Instead, they are practically disabled by setting them to dummy entries which do not affect
     * the message-passing.
     * 
     * If this is specific to an algorithm, it is recommended to call the function in the
     * corresponding algorithm's constructor.
     */
    void use_edge_deduplication();

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // (Abstract) PPGA-paradigm methods defining the graph algorithm: ////////////////////////////////
  // These need to be implemented (or, if available, the default implementation will be used) in  //
  // order to define a custom graph algorithm.                                                    //
  //////////////////////////////////////////////////////////////////////////////////////////////////

  protected:

    size_t size; /// graph size |V|+|E|
    size_t nodes; /// number of nodes |V|
    size_t bits; /// number of bits used to represent node ids
    size_t depth; /// iteration depth

    /**
     * @brief Interface to define initial node values. Optional.
     * 
     * This method can be overridden to set the initial node states before the message-passing
     * begins.
     * 
     * The default implementation returns std::nullopt, in which case the initial node states are
     * not set to static values, but instead are taken from the inputs.
     * 
     * The recommended use is to simply utilize TODO to initialize the node states to a fixed
     * vector of size 'nodes'. Note that when this method is called, it can be assumed that all
     * nodes are sorted, so the vector entries should be arranged appropriately.
     * 
     * @param column id of the column to be initialized, so that multi-column algorithms can use
     *                  different starting values for different columns.
     * @return std::optional<mp_val> Initial state (wire ID) or std::nullopt if inputs shall be used
     *                  as initial state.
     */
    virtual std::optional<mp_val> init_nodes(size_t column);

    /**
     * @brief Preparation of a message to be propagated. Optional.
     * 
     * This method can be overridden to define the message to be passed from a node, depending on
     * the current state.
     * 
     * The default implementation uses the node's current state as message.
     * 
     * @param state current node state
     * @param i number of the current iteration, allows to use different prepares for different
     *                  iterations.
     * @param column id of the column to be initialized, so that multi-column algorithms can use
     *                  different prepares for different columns.
     * @return mp_val Message (wire ID) to be propagated to all outgoing edges.
     */
    virtual mp_val prepare(mp_val &state, size_t i, size_t column);

    // Propagate and Gather are static, hence, there is no interface for customization.
    
    /**
     * @brief Apply of an incoming update to the current node state.
     * 
     * This method must be overridden to define the next node state, given the current node state
     * and the gathered update from the incoming edges.
     * 
     * @param state Current node state
     * @param update Gathered update value
     * @param i number of the current iteration, allows to use different applys for different
     *                  iterations.
     * @param column id of the column to be initialized, so that multi-column algorithms can use
     *                  different applys for different columns.
     * @return mp_val Value (wire ID) to be used as new node state
     */
    virtual mp_val apply(mp_val &state, mp_val &update, size_t i, size_t column);

    /**
     * @brief Post-processing on node states. Optional.
     * 
     * This method can be overridden to define a final operation to be computed on each node state
     * before outputting node states.
     * 
     * The default implementation returns the current state, i.e., does not do any changes.
     * 
     * @param state Current node state
     * @param column id of the column to be initialized, so that multi-column algorithms can use
     *                  different applys for different columns.
     * @return mp_val Value (wire ID) to be used as new node state
     */
    virtual mp_val post_mp(mp_val &state, size_t column);

    /**
     * @brief Final post-processing to aggregate multipel columns into one. Optional.
     * 
     * This method can be overridden to aggregate node states/data from multiple columns into a
     * single column that is used as output. Hence, it takes a vector of the states for all columns
     * as its input.
     * 
     * The default implementation returns std::nullopt, in which case the columns are not aggregated
     * and data from each individual column is send as output.
     * 
     * @param states states of the different columns
     * @return std::optional<mp_val> Value (wire ID) to be used as single-column output or
     *                                  std::nullopt if inputs shall be used  as initial state.
     */
    virtual std::optional<mp_val> post_mp_aggregate(std::vector<mp_val> &states);

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Libary of operations that can be utilized to implement the PPGA-paradigm methods:            //
  //////////////////////////////////////////////////////////////////////////////////////////////////

  protected:

    /**
     * @brief Multiplies two secret values
     * 
     * @param x Secret factor 1
     * @param y Secret factor 2
     * @return mp_val Secret product
     */
    inline mp_val multiply(mp_val &x, mp_val &y) {
        return mul_SIMD(x, y, nodes);
    }

    /**
     * @brief Multiplies a secret with a public constant value
     * 
     * @param x Secret factor 1
     * @param constant Public constant factor 2
     * @return mp_val Secret product
     */
    inline mp_val multiply_by_constant(mp_val &x, Ring constant) {
        return mul_const_SIMD(x, constant);
    }

    /**
     * @brief Adds two secret values
     * 
     * @param x Secret summand 1
     * @param y Secret summand 2
     * @return mp_val Secret sum
     */
    inline mp_val add(mp_val &x, mp_val &y) {
        return add_SIMD(x, y);
    }

    /**
     * @brief Subtracts a secret value from another
     * 
     * @param x Secret value x
     * @param y Secret value y
     * @return mp_val Secret difference x-y
     */
    inline mp_val subtract(mp_val &x, mp_val &y) {
        return sub_SIMD(x, y);
    }

    /**
     * @brief Adds a secret and a public constant value
     * 
     * @param x Secret summand 1
     * @param constant Public constant summand 2
     * @return mp_val Secret sum
     */
    inline mp_val add_constant(mp_val &x, Ring constant) {
        return add_const_SIMD(x, constant);
    }

    /**
     * @brief Flips secrets 0 to 1 and 1 to 0
     * 
     * More exactly, computes 1-x also for inputs >1
     * 
     * @param x Secret input x
     * @return mp_val Secret 1-x
     */
    inline mp_val flip(mp_val &x) {
        return flip_SIMD(x);
    }

    /**
     * @brief Clips secret value, returning secret 0 if input was 0 and 1 otherwise
     * 
     * @param x Secret input x
     * @return mp_val Secret 0 if x = 0 and secret 1 otherwise
     */
    inline mp_val clip(mp_val &x) {
        return clip_SIMD(x);
    }
    
    /**
     * @brief Returns secret states/messages/updates where the entry for the i'th node is set to the
     *          i'th entry of public vector vals
     * 
     * @param vals Public values to set secret states/messages/updates to
     * @return mp_val Set of secret states/messages/updates
     */
    inline mp_val set_const_vector(std::vector<Ring> &vals) {
        return set_const_vec_SIMD(vals);
    }

    /**
     * @brief Computes single set of states where the state of the i'th node is set to the sum over
     *          all node states of column i.
     * 
     * Only use if columns was manually set to a value >1.
     * 
     * @param parallel_data Sets of secret states for all columns.
     * @return mp_val Secret, single, aggregated set of states.
     */
    inline mp_val column_sums_to_nodes(std::vector<mp_val> &parallel_data) {
        assert(columns > 1);
        return column_sums_to_nodes_internal(parallel_data);
    }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Methods utilized by the PPGA core, not relevant when implementing custom graph algorithms: ////
  //////////////////////////////////////////////////////////////////////////////////////////////////

  public:

    size_t n_shuffles; /// Number of currently required shuffling operations
    size_t n_mults; /// Number of currently required multiplication or other interactive gates
    size_t n_wires; /// Current number of used wires
    size_t shuffle_idx; /// Next free id for a distinct shuffling permutation
    size_t columns; /// Number of used columns

    /**
     * Returns the entire circuit, as vector of layers, each layer being a vector of pointers to
     * individual gates.
     * 
     * @return std::vector<std::vector<std::shared_ptr<Gate>>> The circuit
     */
    std::vector<std::vector<std::shared_ptr<Gate>>> get() { return circ; }

  private:

    /// Circuit represented as vector of layers, each being a vector of pointers to individual gates
    std::vector<std::vector<std::shared_ptr<Gate>>> circ;
    /// All gates of the circuit
    std::vector<std::shared_ptr<Gate>> gates;

    MPContext ctx; /// Context, maintaining permutation ids for the specific orders, etc.
    Inputs in; /// Inputs provided to the circuit
   
    /**
     * @brief Operations to be run before computing the individual sorting permutations.
     * 
     * This is currently only being used for parts of the deduplication process.
     */
    void pre_mp();

    /**
     * @brief Computes the individual sorting permutations.
     * 
     * This computes the permutations required to switch between input, vertex, source, and
     * destination orders.
     */
    void compute_sorts();

    /**
     * @brief Assigns all gates to circuit layers
     * 
     * This automatically runs the layering of the circuit, where each layer contains the maximum of
     * gates where any interactive gate on the layer must not have the output of another gate on the
     * same layer as output. The layers later correspond to the online communication rounds 1-to-1.
     */
    void level_order();

    /**
     * @brief Creates all input gates
     * 
     * This method creates all gates required for the graph input.
     */
    void set_inputs();

    /**
     * Prepares the shufflings for switching between DAG lsit orders in each iteration.
     */
    void prepare_shuffles();

    /**
     * Generates the message-passing subcircuit for one column
     * 
     * @param data Input DAG list node state data
     * @param column Column id if message-passing is done on multiple columns
     * @return SIMD_wire_id Resulting node state data
     */
    SIMD_wire_id message_passing(SIMD_wire_id &data, size_t column);

    /**
     * Computes the sorting permutation with keys already decomposed into 'bits' many bits
     * 
     * @param bit_keys vector of LSB up to MS(used)B
     * @param bits Number of bits, normally the length of aforementioned vector
     * @return SIMD_wire_id resulting sorting permutation
     */
    SIMD_wire_id sort(std::vector<SIMD_wire_id> &bit_keys, SIMD_wire_id bits);

    /**
     * Single iteration of the sorting procedure
     * 
     * @param perm Current sorting permutation
     * @param keys Next bit to sort by
     * @return SIMD_wire_id Resulting sorting permutation
     */
    SIMD_wire_id sort_iteration(SIMD_wire_id &perm, SIMD_wire_id &keys);

    /// flag to force switching back to input order before outputting
    bool outputs_in_input_order = false;
    /// flag to block outputs_in_input_order from being set after build() has been called
    bool can_enable_outputs_in_input_order = true;
    /// flag to make message-passing traverse edges backwards
    bool reverse_passing = false;
    /// flag to block reverse_passing from being set after build() has been called
    bool can_enable_reverse_passing = true;
    /// flag to deduplicate potentially duplicate edges before running the message-passing
    bool deduplication = false;
    /// flag to block deduplication from being set after build() has been called
    bool can_enable_deduplication = true;

    // Primitives:
    // (each may have SIMD input wires and may have a SIMD output wire, in which case it's returned)

    /// Creates a SIMD input gate
    SIMD_wire_id input();
    /// Creates a SIMD output gate
    void output(SIMD_wire_id &input);
    /// Generates subcircuit to switch to input order (if needed) and sanitize edge data before
    /// finally appending a SIMD output gate
    void output_data(SIMD_wire_id &input);
    /// Step 1 of propagating, run in vertex order
    SIMD_wire_id propagate_1(SIMD_wire_id &input); 
    /// Step 2 of propagating, run in source order
    SIMD_wire_id propagate_2(SIMD_wire_id &input1, SIMD_wire_id &input2);
    /// Step 1 of gathering, run in destination order
    SIMD_wire_id gather_1(SIMD_wire_id &input);
    /// Step 2 of gathering, run in vertex order
    SIMD_wire_id gather_2(SIMD_wire_id &input);
    /// Shuffles the input using the given shuffling permutation
    SIMD_wire_id shuffle(SIMD_wire_id &input, SIMD_wire_id shuffle_idx);
    /// Unshuffles the input using the given shuffling permutation
    SIMD_wire_id unshuffle(SIMD_wire_id &input, SIMD_wire_id shuffle_idx);
    /// Unshuffles using pi and then shuffles using omega, designating shuffle_idx as the composition
    SIMD_wire_id merged_shuffle(SIMD_wire_id &input, SIMD_wire_id shuffle_idx, SIMD_wire_id pi_idx, SIMD_wire_id omega_idx);
    /// Adds a compaction gate
    SIMD_wire_id compaction(SIMD_wire_id &input);
    /// Adds a reveal gate, revealing a (masked) intermediate value in plain, not to be used as an
    /// output!
    SIMD_wire_id reveal(SIMD_wire_id &input);
    /// Adds a subcircuit to permute the given input with the given permutation
    SIMD_wire_id permute(SIMD_wire_id &input, SIMD_wire_id &perm);
    /// Adds a subcircuit to permute the given input with the inverse of the given permutation
    SIMD_wire_id reverse_permute(SIMD_wire_id &input, SIMD_wire_id &perm);
    /// Adds subcircuit to compute (in binary domain) a bit 1 if the input is =0 and bit 0 otherwise
    SIMD_wire_id equals_zero(SIMD_wire_id &input, SIMD_wire_id size);
    /// Subcomponent for equals_zero
    SIMD_wire_id _equals_zero(SIMD_wire_id &input, SIMD_wire_id size, SIMD_wire_id layer);
    /// Converts the binary domain input to an arithmetic sharing
    SIMD_wire_id bit2A(SIMD_wire_id &input, SIMD_wire_id size);
    /// Deduplication subprocedure
    SIMD_wire_id deduplication_sub(SIMD_wire_id &input1);
    /// Deduplication subprocedure
    SIMD_wire_id deduplication_insert(SIMD_wire_id &input1);

    /// Secret multiplication (or AND, if binary = true)
    SIMD_wire_id mul_SIMD(SIMD_wire_id &x, SIMD_wire_id &y, bool binary = false);
    /// Secret multiplication (or AND, if binary = true) only on the first size many entries.
    SIMD_wire_id mul_SIMD(SIMD_wire_id &x, SIMD_wire_id &y, SIMD_wire_id size, bool binary = false);
    /// Multiplication of a secret and a scalar public constant
    SIMD_wire_id mul_const_SIMD(SIMD_wire_id &data, Ring val);
    /// Secret addition
    SIMD_wire_id add_SIMD(SIMD_wire_id &input1, SIMD_wire_id &input2);
    /// Secret subtraction
    SIMD_wire_id sub_SIMD(SIMD_wire_id &input1, SIMD_wire_id &input2);
    /// Addition of a secret and a scalar public constant
    SIMD_wire_id add_const_SIMD(SIMD_wire_id &data, Ring val);
    /// Maps 1 to 0 and 0 to 1
    SIMD_wire_id flip_SIMD(SIMD_wire_id &input);
    /// Maps 0 to 0 and any other value >0 to 1
    SIMD_wire_id clip_SIMD(SIMD_wire_id &data);
    /// Creates wire holding the given public value vector in secret-shared form
    SIMD_wire_id set_const_vec_SIMD(std::vector<Ring> &vals);
    /// Computes vector where entry i contains the sum over all entries of column i.
    /// Only use if columns > 1!
    SIMD_wire_id column_sums_to_nodes_internal(std::vector<SIMD_wire_id> &parallel_data);
};