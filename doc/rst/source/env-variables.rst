=====================
Environment Variables
=====================

.. _collective-algorithms-selection:

Collective Algorithms Selection
###############################

oneCCL supports collective operations for the host (CPU) memory buffers and
device (GPU) memory buffers. In addition, oneCCL has two different paths to
support collectives with GPU buffers; one directly uses Level Zero, and the
other uses SYCL. The SYCL path is a new code being developed and not all
collectives are supported.

For the Level Zero implementation, in the case of GPU buffers, oneCCL
collectives are optimized to execute a hierarchical algorithm composed of an
optimized scale-up phase (communication between ranks/processes in the same
node) and a scaleout phase (communication between ranks/processes on different
nodes). In the case of CPU buffers, the current collective algorithms do not
have support for scale-up and scaleout phases; only a non-hierarchical
algorithm can be chosen.

With ``CCL_<coll_name> = <algo_name>``, you can select the algorithm for the
collective in ``<coll_name>``. For GPU buffers, the default algorithm is
``topo``, which refers to the scale-up algorithm. If you select an algorithm
different from ``topo``, oneCCL will implement a non-hierarchical algorithm,
where it will copy the GPU buffers to the Host (CPU) and will run the specified
algorithm.

For CPU buffers, ``topo`` is not available; you can only select one of the
other algorithms in the table for a given collective.

If the collective uses GPU buffers, you can select whether the implementation
of the scale-up algorithm should use copy engines or kernels. There is also the
option to select the scaleout algorithm using
``CCL_<coll_name>_SCALEOUT=<algo_name>``.

Next, environment variables for collective algorithm selection are explained
based on the code path (Level Zero or SYCL), the collective being called, and
the type of buffer (GPU or CPU).


Level Zero Path (Default)
*************************

ALLGATHERV
==========

CCL_ALLGATHERV
--------------

**Syntax**

For the whole message size:

::

  CCL_ALLGATHERV=<algo_name>

For a specific message size range:

::

  CCL_ALLGATHERV="<algo_name_1>[:<size_range_1>][;<algo_name_2>:<size_range_2>][;...]"

Where:

* ``<algo_name>`` is selected from the list of the available collective
  algorithms.
* ``<size_range>`` is described by the left and the right size
  borders in the ``<left>-<right>`` format. The size is specified in bytes. To
  specify the maximum message size, use the reserved word max.

**Example**

::

  CCL_ALLGATHERV="direct:0-8192;ring:8193-max"

**Arguments**

.. list-table::
   :widths: 25 50
   :header-rows: 1
   :align: left

   * - <algo_name>
     - Description
   * - ``topo``
     - topology-aware algorithm for scale-up. The default for GPU buffers. Not available for CPU buffers.
   * - ``direct``
     - Based on ``MPI_Iallgatherv``.
   * - ``naive``
     - Send to all, receive from all.
   * - ``flat``
     - alltoall-based algorithm.
   * - ``multi_bcast``
     - Series of broadcast operations with different root ranks.
   * - ``ring``
     - ring-based algorithm.


**Description**

Use this environment variable to specify the algorithm for ALLGATHERV.

If using GPU buffers, select ``CCL_ALLGATHER=topo`` (the default) to use a hierarchical algorithm for scale-up data transfer across GPUs in the same node.
For GPU buffers, when selecting an algorithm different from ``topo``, oneCCL copies the data to the host and follows the specified CPU algorithm.


CCL_ALLGATHERV_MONOLITHIC_PIPELINE_KERNEL
-----------------------------------------

**Syntax**

::

  CCL_ALLGATHERV_MONOLITHIC_PIPELINE_KERNEL=<value>

**Arguments**

.. list-table::
   :widths: 25 50
   :header-rows: 1
   :align: left

   * - <value>
     - Description
   * - ``1``
     - Uses compute kernels to transfer data across GPUs for the allgather phase of ``ALLGATHERV``. The default value.
   * - ``0``
     - Uses copy engines to transfer data across GPUs for the allgather phase of the ``ALLGATHERV`` collective.

**Description**

Set this environment variable to use GPU buffers to specify the scale-up phase of the algorithm for ALLGATHERV.
This environment variable allows the user to choose between using compute kernels or copy engines.

This option is only available if ``CCL_ALLGATHERV=topo`` (the default for GPU buffers).



CCL_ALLGATHERV_SCALEOUT
-----------------------

**Syntax**

For the whole message size:

::

  CCL_ALLGATHER_SCALEOUT


For a specific message size range:
::

  CCL_ALLGATHERV_SCALEOUT="<algo_name_1>[:<size_range_1>][;<algo_name_2>:<size_range_2>][;...]"

Where:

* ``<algo_name>`` is selected from the list of the available scaleout
  collective algorithms.
* ``<size_range>`` is described by the left and the
  right size borders in the ``<left>-<right>`` format. The size is specified in
  bytes. To specify the maximum message size, use the reserved word max.

**Example**

::

  CCL_ALLGATHERV_SCALEOUT="direct:0-8192;ring:8193-max"

**Arguments**

.. list-table::
   :widths: 25 50
   :header-rows: 1
   :align: left

   * - <algo_name>
     - Description
   * - ``direct``
     - Based on ``MPI_Iallgatherv``.
   * - ``naive``
     - Send to all, receive from all.
   * - ``flat``
     - alltoall-based algorithm.
   * - ``multi_bcast``
     - Series of broadcast operations with different root ranks.
   * - ``ring``
     - ring-based algorithm. The default value.

**Description**

Set this environment variable to use GPU buffers to specify the scaleout phase of the algorithm for ALLGATHERV.
This option is only available if ``CCL_ALLGATHERV = topo`` (the default for GPU buffers).

oneCCL internally fills the algorithm selection table with appropriate defaults. Your input complements the selection table.

To see the actual table values, set ``CCL_LOG_LEVEL=info``.

ALLREDUCE
=========

CCL_ALLREDUCE
-------------

**Syntax**

For the whole message size:

::

  CCL_ALLREDUCE=<algo_name>

For a specific message size range:

::

  CCL_ALLREDUCE="<algo_name_1>[:<size_range_1>][;<algo_name_2>:<size_range_2>][;...]"

Where:

* ``<algo_name>`` is selected from the list of available collective algorithms.
* ``<size_range>`` is described by the left and the right size
  borders in the ``<left>-<right>`` format. The size is specified in bytes. To
  specify the maximum message size, use the reserved word max.

**Example**

::

  CCL_ALLREDUCE="recursive_doubling:0-8192;rabenseifner:8193-1048576;ring:1048577-max"

**Arguments**

.. list-table::
   :widths: 25 50
   :align: left

   * - <algo_name>
     - Description
   * - ``topo``
     - topology-aware algorithm for scale-up. The default for GPU buffers. Not available for CPU buffers.
   * - ``direct``
     - Based on ``MPI_Iallreduce``.
   * - ``rabenseifner``
     - Rabenseifner algorithm.
   * - ``nreduce``
     - May be beneficial for imbalanced workloads.
   * - ``ring``
     - reduce_scatter + allgather ring. Use CCL_RS_CHUNK_COUNT and CCL_RS_MIN_CHUNK_SIZE to control pipelining on reduce_scatter phase.
   * - ``double_tree``
     - double-tree algorithm.
   * - ``recursive_doubling``
     - Recursive doubling algorithm.
   * - ``2d``
     - Two-dimensional algorithm (reduce_scatter + allreduce + allgather).

**Description**

Use this environment variable to specify the algorithm for ALLREDUCE.

If using GPU buffers, select ``CCL_ALLREDUCE=topo`` (the default) to use a hierarchical algorithm for scale-up data transfer across GPUs in the same node.
For GPU buffers, when selecting an algorithm different from ``topo``, oneCCL copies the data to the host and follows the specified CPU algorithm.

oneCCL internally fills the algorithm selection table with appropriate defaults. Your input complements the selection table.

To see the actual table values, set ``CCL_LOG_LEVEL=info``.


CCL_REDUCE_SCATTER_MONOLITHIC_PIPELINE_KERNEL (GPU buffers only)
----------------------------------------------------------------

**Syntax**

::

 CCL_REDUCE_SCATTER_MONOLITHIC_PIPELINE_KERNEL=<value>

**Arguments**

.. list-table::
   :widths: 25 50
   :header-rows: 1
   :align: left

   * - <value>
     - Description
   * - ``1``
     - Uses compute kernels to transfer data across GPUs for the reduce-scatter phase of the  ``ALLREDUCE`` collectives. The default value.
   * - ``0``
     - Uses copy engines to transfer data across GPUs for the reduce-scatter phase of the ``ALLREDUCE``.

**Description**

Set this environment variable to use GPU buffers to specify how to perform the reduce_scatter portion of the scale-up ``ALLREDUCE`` collective.
This variable allows you to choose between using compute kernels or copy engines.

This option is only available if ``CCL_ALLREDUCE=topo`` (the default for GPU buffers).



CCL_ALLGATHERV_MONOLITHIC_PIPELINE_KERNEL (GPU buffers only)
------------------------------------------------------------

**Syntax**

::

  CCL_ALLGATHERV_MONOLITHIC_PIPELINE_KERNEL=<value>

**Arguments**

.. list-table::
   :widths: 25 50
   :header-rows: 1
   :align: left

   * - <value>
     - Description
   * - ``1``
     - Uses compute kernels to transfer data across GPUs for the allgather phase of ``ALLREDUCE``. The default value.
   * - ``0``
     - Uses copy engines to transfer data across GPUs for the allgather phase of the ``ALLREDUCE`` collective.

**Description**

ALLREDUCE is implemented as a reduce-scatter phase followed by an allgather phase.

Set this environment variable to use GPU buffers to specify how to perform the
allgather portion of the scale-up ``ALLREDUCE`` collective. This environment
variable allows the user to choose between using compute kernels or using copy
engines. This option is only available if ``CCL_ALLGATHERV=topo`` (the default
for GPU buffers).


CCL_ALLREDUCE_SCALEOUT (GPU buffers only)
-----------------------------------------

**Syntax**

For the whole message size:

::

  CCL_ALLREDUCE_SCALEOUT=<algo_name>

For a specific message size range:

::

  CCL_ALLREDUCE_SCALEOUT="<algo_name_1>[:<size_range_1>][;<algo_name_2>:<size_range_2>][;...]"

Where:

* ``<algo_name>`` is selected from the list of available collective algorithms.
* ``<size_range>`` is described by the left and the right size borders the
  ``<left>-<right>`` format. The size is specified in bytes. To specify the maximum message size, use the reserved word max.

**Example**

::

  CCL_ALLREDUCE_SCALEOUT="recursive_doubling:0-8192;rabenseifner:8193-1048576;ring:1048577-max

**Arguments**

.. list-table::
   :widths: 25 50
   :align: left

   * - ``direct``
     - Based on ``MPI_allreduce``
   * - ``rabenseifner``
     - Rabenseifner algorithm.
   * - ``nreduce``
     - May be beneficial for imbalanced workloads.
   * - ``ring``
     - reduce_scatter + allgather ring. Use ``CCL_RS_CHUNK_COUNT`` and ``CCL_RS_MIN_CHUNK_SIZE`` to control pipelining on reduce_scatter phase. The default value.
   * - ``double_tree``
     - double-tree algorithm.
   * - ``ring``
     - Recursive doubling algorithm.

**Description**

Set this environment variable to use GPU buffers to specify the scaleout algorithm for ALLREDUCE.
This option is only available if ``CCL_ALLREDUCE = topo`` (the default for GPU buffers).

oneCCL internally fills the algorithm selection table with appropriate defaults. Your input complements the selection table.

To see the actual table values, set ``CCL_LOG_LEVEL=info``.

ALLTOALL, ALLTOALLV
===================

CCL_ALLTOALL, CCL_ALLTOALLV
---------------------------

**Syntax**

For the whole message size:

::

  CCL_ALLTOALL=<algo_name>  or CCL_ALLTOALLV=<algo_name>

For a specific message size range:

::

  CCL_ALLTOALL="<algo_name_1>[:<size_range_1>][;<algo_name_2>:<size_range_2>][;...]"

or

::

  CCL_ALLTOALLV="<algo_name_1>[:<size_range_1>][;<algo_name_2>:<size_range_2>][;...]"


Where:

* ``<algo_name>`` is selected from the list of available collective algorithms.
* ``<size_range>`` is described by the left and the right size borders in the
  ``<left>-<right>`` format. The size is specified in bytes. To specify the maximum message size, use the reserved word max.

**Example**

::

  CCL_ALLTOALL="naive:0-8192;scatter:8193-max"

or

::

  CCL_ALLTOALLV="naive:0-8192;scatter:8193-max"

**Arguments**

.. list-table::
   :widths: 25 50
   :align: left

   * - ``topo``
     - topology-aware algorithm. The default for GPU buffers. Not available for CPU buffers.
   * - ``direct``
     - Based on ``MPI_Ialltoall``
   * - ``naive``
     - Send to all, receive from all.
   * - ``scatter``
     - scatter-based algorithm.


CCL_ALLTOALLV_MONOLITHIC_KERNEL
-------------------------------

**Syntax**

::

  CCL_ALLTOALLV_MONOLITHIC_KERNEL=<value>

**Arguments**

.. list-table::
   :widths: 25 50
   :header-rows: 1
   :align: left

   * - <value>
     - Description
   * - ``1``
     - Uses compute kernels to transfer data across GPUs for the allgather phase of the ``ALLTOALL`` and ``ALLTOALLV`` collectives. The default value.
   * - ``0``
     - Uses copy engines to transfer data across GPUs for the allgather phase of the ``ALLTOALL`` and ``ALLTOALLV`` collectives.

**Description**

Set this environment variable to use GPU buffers to specify the scale-up algorithm for ``ALLTOALL`` or ``ALLTOALLV``
This environment variable allows the user to choose between using compute kernels or using copy engines.

This option is only available if ``CCL_ALLTOALL=topo`` or ``CCL_ALLTOALLV=topo``. The default for GPU buffers.

CCL_ALLTOALL_SCALEOUT, CCL_scaleout_ALLTOALLV_scaleout
--------------------------------------------------------

**Syntax**

For the whole message size:

::

  CCL_ALLTOALL_SCALEOUT=<algo_name>  or CCL_ALLTOALLV_SCALEOUT=<algo_name>


For a specific message size range:

::

  CCL_ALLTOALL_SCALEOUT="<algo_name_1>[:<size_range_1>][;<algo_name_2>:<size_range_2>][;...]"

or

::

  CCL_ALLTOALLV_SCALEOUT="<algo_name_1>[:<size_range_1>][;<algo_name_2>:<size_range_2>][;...]"

Where:

* ``<algo_name>`` is selected from the list of available collective algorithms.
* ``<size_range>`` is described by the left and the right size borders in a
  format ``<left>-<right>``. The size is specified in bytes. To specify the maximum message size, use the reserved word max.

**Example**

::

  CCL_ALLTOALL_SCALEOUT="naive:0-8192;scatter:8193-max"

or

::

  CCL_ALLTOALLV_SCALEOUT="naive:0-8192;scatter:8193-max"

**Arguments**

.. list-table::
   :widths: 25 50
   :header-rows: 1
   :align: left

   * - <algo_name>
     - Description
   * - ``naive``
     - Send to all, receive from all.
   * - ``scatter``
     - scatter-based algorithm. The default value.

**Description**

Set this environment variable to use GPU buffers to specify the scaleout algorithm for ``ALLTOALL`` or ``ALLTOALLV``.
This option is only available if ``CCL_ALLTOALL=topo`` or ``CCL_ALLTOALLV=topo`` (the default for GPU buffers).

oneCCL internally fills the algorithm selection table with appropriate defaults. Your input complements the selection table.

To see the actual table values, set ``CCL_LOG_LEVEL=info``.

BARRIER
=======

CCL_BARRIER
-----------

**Syntax**

::

  CCL_BARRIER=<algo_name>

**Arguments**

.. list-table::
   :widths: 25 50
   :header-rows: 1
   :align: left

   * - <algo_name>
     - Description
   * - ``direct``
     - Based on ``MPI_Ibarrier``.
   * - ``ring``
     - Ring-based algorithm.

**Description**

Use this environment variable to select the barrier algorithm.

BROADCAST
=========

CCL_BCAST
---------

**Syntax**

::

  CCL_BCAST=<algo_name>

**Arguments**

.. list-table::
   :widths: 25 50
   :align: left

   * - <algo_name>
     - Description
   * - ``topo``
     - topology-aware algorithm. The default for GPU buffers. Not available for CPU buffers.
   * - ``direct``
     - Based on MPI_Ibcast.
   * - ``ring``
     - ring-based algorithm.
   * - ``double_tree``
     - double-tree algorithm.
   * - ``naive``
     - Send to all from root rank.

**Description**

Use this environment variable to select the algorithm used for broadcast.

.. note::

  The ``BCAST`` algorithm does not yet support the ``CCL_BCAST_scaleout``
  environment variable. To change the algorithm for ``BCAST``, use the ``CCL_BCAST`` environment variable.

REDUCE
======

CCL_REDUCE
----------

**Syntax**

For the whole message size:

::

  CCL_REDUCE=<algo_name>

For a specific message size range:

::

  CCL_REDUCE="<algo_name_1>[:<size_range_1>][;<algo_name_2>:<size_range_2>][;...]"

Where:

* ``<algo_name>`` is selected from the list of available collective algorithms.
* ``<size_range>`` is described by the left and the right size borders in the
  ``<left>-<right>`` format. The size is specified in bytes. To specify the maximum message size, use the reserved word max.


**Example**

::

  CCL_REDUCE="direct:0-8192;double_tree:1048577-max"

**Arguments**

.. list-table::
   :widths: 25 50
   :align: left

   * - <algo_name>
     - Description
   * - ``topo``
     - topology-aware algorithm for scale-up. The default for GPU buffers. Not available for CPU buffers.
   * - ``direct``
     - Based on ``MPI_Ireduce``.
   * - ``rabenseifner``
     - Rabenseifner algorithm.
   * - ``tree``
     - tree algorithm
   * - ``double_tree``
     - double-tree algorithm.


**Description**

Set this environment variable to specify the algorithm for ``REDUCE``.

If using GPU buffers, select ``CCL_REDUCE=topo`` (the default) to use a hierarchical algorithm for scale-up data transfer across GPUs in the same node.
For GPU buffers, when selecting an algorithm different from ``topo``, oneCCL copies the data to the host and follows the specified CPU algorithm.

oneCCL internally fills the algorithm selection table with appropriate defaults. Your input complements the selection table.

To see the actual table values, set ``CCL_LOG_LEVEL=info``.

CCL_REDUCE_SCATTER_MONOLITHIC_PIPELINE_KERNEL (GPU buffers only)
----------------------------------------------------------------

**Syntax**

::


  CCL_REDUCE_SCATTER_MONOLITHIC_PIPELINE_KERNEL=<value>

**Arguments**

.. list-table::
   :widths: 25 50
   :align: left

   * - <value>
     - Description
   * - ``1``
     - Uses compute kernels to transfer data across GPUs for the reduce-scatter phase of the ``REDUCE`` collective. The default value.
   * - ``0``
     - Uses copy engines to transfer data across GPUs for the reduce-scatter phase of the ``REDUCE`` collective.

**Description**

Set this environment variable to use GPU buffers to specify the scale-up algorithm for ALLREDUCE.
This environment variable allows the user to choose between using compute kernels or using copy engines.

This option is only available if ``CCL_REDUCE=topo`` (the default for GPU buffers).

CCL_REDUCE_SCALEOUT (GPU buffers only)
--------------------------------------

**Syntax**

For the whole message size:

::

  CCL_REDUCE_SCALEOUT=<algo_name>

For a specific message size range:

::

 CCL_REDUCE_SCALEOUT="<algo_name_1>[:<size_range_1>][;<algo_name_2>:<size_range_2>][;...]"

Where:

* ``<algo_name>`` is selected from the list of available collective algorithms.
* ``<size_range>`` is described by the left and the right size borders in
  a format ``<left>-<right>``. The size is specified in bytes. To specify the maximum message size, use the reserved word max.

**Example**

::

  CCL_REDUCE_SCALEOUT="direct:0-8192;double_tree:1048577-max"

**Arguments**

.. list-table::
   :widths: 25 50
   :align: left

   * - <algo_name>
     - Description
   * - ``direct``
     - Based on ``MPI_Ireduce``.
   * - ``rabenseifner``
     - Rabenseifner algorithm.
   * - ``tree``
     - tree algorithm.
   * - ``double_tree``
     - double-tree algorithm. The default value.


**Description**

Set this environment variable to use GPU buffers to specify the scaleout algorithm for ``REDUCE``.
This option is only available if ``CCL_REDUCE=topo`` (the default for GPU buffers).

oneCCL internally fills the algorithm selection table with appropriate defaults. Your input complements the selection table.

To see the actual table values, set ``CCL_LOG_LEVEL=info``.

REDUCE_SCATTER
==============

CCL_REDUCE_SCATTER
------------------

**Syntax**

For the whole message size:

::

 CCL_REDUCE_SCATTER=<algo_name>

For a specific message size range:

::

 CCL_REDUCE_SCATTER="<algo_name_1>[:<size_range_1>][;<algo_name_2>:<size_range_2>][;...]"

Where:

* ``<algo_name>`` is selected from the list of available collective algorithms.
* ``<size_range>`` is described by the left and the right size borders in a
  format ``<left>-<right>``. The size is specified in bytes. To specify the maximum message size, use the reserved word max.


**Example**

::

  CCL_REDUCE_SCATTER="direct:0-8192;ring:1048577-max"

**Arguments**

.. list-table::
   :widths: 25 50
   :align: left

   * - <algo_name>
     - Description
   * - ``topo``
     - topology-aware algorithm for scale-up. The default for GPU buffers. Not available for CPU buffers.
   * - ``direct``
     - Based on ``MPI_Ireduce_scatter_block``.
   * - ``naive``
     - Send to all, receive, and reduce from all.
   * - ``ring``
     - ring-based algorithm. Use ``CCL_RS_CHUNK_COUNT`` and ``CCL_RS_MIN_CHUNK_SIZE`` to control pipelining.


**Description**

Use this environment variable to specify the algorithm for reduce. If using GPU
buffers, select ``CCL_REDUCE_SCATTER=topo`` (the default) to use a hierarchical
algorithm for scale-up data transfer across GPUs in the same node. For GPU
buffers,when selecting an algorithm different from ``topo``, oneCCL copies the
data to the host and follow the specified CPU algorithm.

oneCCL internally fills the algorithm selection table with appropriate defaults. Your input complements the selection table.

To see the actual table values, set ``CCL_LOG_LEVEL=info``.

CCL_REDUCE_SCATTER_MONOLITHIC_PIPELINE_KERNEL (GPU buffers only)
----------------------------------------------------------------

**Syntax**

::


  CCL_REDUCE_SCATTER_MONOLITHIC_PIPELINE_KERNEL=<value>

**Arguments**

.. list-table::
   :widths: 25 50
   :align: left

   * - <value>
     - Description
   * - ``1``
     - Uses compute kernels to transfer data across GPUs for the reduce-scatter phase of the ``REDUCE_SCATTER`` collective. The default value.
   * - ``0``
     - Uses copy engines to transfer data across GPUs for the reduce-scatter phase of the ``REDUCE_SCATTER`` collective.

**Description**

Set this environment variable to use GPU buffers to specify how to perform the reduce-scatter portion of the scale-up ``REDUCE_SCATTER`` collective.
This environment variable allows the user to choose between using compute kernels or using copy engines.

This option is only available if ``CCL_REDUCE_SCATTER=topo``  (the default for GPU buffers).

CCL_REDUCE_SCATTER_SCALEOUT (GPU buffers only)
----------------------------------------------

**Syntax**

For the whole message size:

::

 CCL_REDUCE_SCATTER_SCALEOUT=<algo_name>

For a specific message size range:

::

  CCL_REDUCE_SCATTER_SCALEOUT="<algo_name_1>[:<size_range_1>][;<algo_name_2>:<size_range_2>][;...]"

Where:

* ``<algo_name>`` is selected from the list of available collective algorithms.
* ``<size_range>`` is described by the left and the right size borders in a
  format ``<left>-<right>``. The size is specified in bytes. To specify the maximum message size, use the reserved word max.

**Example**

::

  CCL_REDUCE_SCATTER_SCALEOUT="direct:0-8192;double_tree:1048577-max"

**Arguments**

.. list-table::
   :widths: 25 50
   :align: left

   * - <algo_name>
     - Description
   * - ``direct``
     - Based on ``MPI_Ireduce_scatter_block``.
   * - ``naive``
     - Send to all, receive, and reduce from all. The default value.
   * - ``ring``
     - Ring-based algorithm. Use ``CCL_RS_CHUNK_COUNT`` and ``CCL_RS_MIN_CHUNK_SIZE`` to control pipelining.


**Description**

Set this environment variable to use GPU buffers to specify the scaleout algorithm for ALLREDUCE.
This option is only available if ``CCL_REDUCE_SCATTER = topo`` (the default for GPU buffers).

oneCCL internally fills the algorithm selection table with appropriate defaults. Your input complements the selection table.

To see the actual table values, set ``CCL_LOG_LEVEL=info``.

SYCL PATH
*********

All collectives
===============

CCL_ENABLE_SYCL_KERNELS
-----------------------

**Syntax**

::


  CCL_ENABLE_SYCL_KERNELS=<value>

**Arguments**

.. list-table::
   :widths: 25 50
   :align: left

   * - <value>
     - Description
   * - ``1``
     - Enable SYCL kernels.
   * - ``0``
     - Disable SYCL kernels. The default value.

**Description**

Setting this environment variable to 1 enables SYCL kernel-based implementations for ``ALLGATHERV``, ``ALLREDUCE``, and ``REDUCE_SCATTER``.

This new optimization optimizes all message sizes and supports the following data types:

* int32
* fp32
* fp16
* bf16
* sum operations
* single nodes

oneCCL falls back to other implementations when the support is unavailable with SYCL kernels, so that you can set up this environment variable safely.

.. note::

  The name of this variable in 2021.12 was ``CCL_SKIP_SCHEDULER``. Starting with 2021.13, the variable has been renamed to ``CCL_ENABLE_SYCL_KERNELS``.

Workers
#######


The group of environment variables to control worker threads.

.. _CCL_WORKER_COUNT:

CCL_WORKER_COUNT
****************
**Syntax**

::

  CCL_WORKER_COUNT=<value>

**Arguments**

.. list-table::
   :widths: 25 50
   :header-rows: 1
   :align: left

   * - <value>
     - Description
   * - ``N``
     - The number of worker threads for |product_short| rank (``1`` if not specified).

**Description**

Set this environment variable to specify the number of |product_short| worker threads.

.. _CCL_WORKER_AFFINITY:

CCL_WORKER_AFFINITY
*******************

**Syntax**

::

  CCL_WORKER_AFFINITY=<cpulist>

**Arguments**

.. list-table::
   :widths: 25 50
   :header-rows: 1
   :align: left

   * - <cpulist>
     - Description
   * - ``auto``
     - Workers are automatically pinned to last cores of pin domain.
       Pin domain depends from process launcher.
       If ``mpirun`` from |product_short| package is used then pin domain is MPI process pin domain.
       Otherwise, pin domain is all cores on the node.
   * - ``<cpulist>``
     - A comma-separated list of core numbers and/or ranges of core numbers for all local workers, one number per worker.
       The i-th local worker is pinned to the i-th core in the list.
       For example ``<a>,<b>-<c>`` defines list of cores containing core with number ``<a>``
       and range of cores with numbers from ``<b>`` to ``<c>``.
       The core number should not exceed the number of cores available on the system. The length of the list should be equal to the number of workers.

**Description**

Set this environment variable to specify cpu affinity for |product_short| worker threads.


CCL_WORKER_MEM_AFFINITY
***********************

**Syntax**

::

  CCL_WORKER_MEM_AFFINITY=<nodelist>

**Arguments**

.. list-table::
   :widths: 25 50
   :header-rows: 1
   :align: left

   * - <nodelist>
     - Description
   * - ``auto``
     - Workers are automatically pinned to NUMA nodes that correspond to CPU affinity of workers.
   * - ``<nodelist>``
     - A comma-separated list of NUMA node numbers for all local workers, one number per worker.
       The i-th local worker is pinned to the i-th NUMA node in the list.
       The number should not exceed the number of NUMA nodes available on the system.

**Description**

Set this environment variable to specify memory affinity for |product_short| worker threads.


ATL
###


The group of environment variables to control ATL (abstract transport layer).


.. _CCL_ATL_TRANSPORT:

CCL_ATL_TRANSPORT
*****************

**Syntax**

::

  CCL_ATL_TRANSPORT=<value>

**Arguments**

.. list-table::
   :widths: 25 50
   :header-rows: 1
   :align: left

   * - <value>
     - Description
   * - ``mpi``
     - MPI transport (**default**).
   * - ``ofi``
     - OFI (libfabric\*) transport.

**Description**

Set this environment variable to select the transport for inter-process communications.


CCL_ATL_HMEM
************
**Syntax**

::

  CCL_ATL_HMEM=<value>

**Arguments**

.. list-table::
   :widths: 25 50
   :header-rows: 1
   :align: left

   * - <value>
     - Description
   * - ``1``
     - Enable heterogeneous memory support on the transport layer.
   * - ``0``
     - Disable heterogeneous memory support on the transport layer (**default**).

**Description**

Set this environment variable to enable handling of HMEM/GPU buffers by the transport layer.
The actual HMEM support depends on the limitations on the transport level and system configuration.

CCL_ATL_SHM
***********

**Syntax**
::

  CCL_ATL_SHM=<value>

**Arguments**

.. list-table::
   :widths: 25 50
   :header-rows: 1
   :align: left

   * - <value>
     - Description
   * - ``0``
     - Disables the OFI shared memory provider. The default value.
   * - ``1``
     - Enables the OFI shared memory provider.

**Description**

Set this environment variable to enable the OFI shared memory provider to
communicate between ranks in the same node of the host (CPU) buffers. This
capability requires OFI as the transport (``CCL_ATL_TRANSPORT=ofi``).

The OFI/SHM provider has support to utilize the `Intel(R) Data Streaming Accelerator* (DSA) <https://01.org/blogs/2019/introducing-intel-data-streaming-accelerator>`_.
To run it with DSA*, you need:
* Linux* OS kernel support for the DSA* shared work queues
* Libfabric* 1.17 or later

To enable DSA, set the following environment variables:

.. code::

    FI_SHM_DISABLE_CMA=1
    FI_SHM_USE_DSA_SAR=1

Refer to Libfabric* Programmer's Manual for the additional details about DSA*
support in the SHM provider:
https://ofiwg.github.io/libfabric/main/man/fi_shm.7.html.

CCL_PROCESS_LAUNCHER
********************

**Syntax**
::

  CCL_PROCESS_LAUNCHER=<value>

**Arguments**

.. list-table::
   :widths: 25 50
   :header-rows: 1
   :align: left

   * - <value>
     - Description
   * - ``hydra``
     - Uses the MPI hydra job launcher. The default value.
   * - ``torch``
     - Uses a torch job launcher.
   * - ``pmix``
     - Is used with the PALS job launcher that uses the pmix API. The ``mpiexec`` command should be similar to:

       ::

         CCL_PROCESS_LAUNCHER=pmix CCL_ATL_TRANSPORT=mpi mpiexec -np 2 -ppn 2 --pmi=pmix ...
   * - ``none``
     - No job launcher is used. You should specify the values for ``CCL_LOCAL_SIZE and CCL_LOCAL_RANK``.


**Description**

Set this environment variable to specify the job launcher.


CCL_LOCAL_SIZE
**************

**Syntax**
::

  CCL_LOCAL_SIZE=<value>

**Arguments**

.. list-table::
   :widths: 25 50
   :header-rows: 1
   :align: left

   * - <value>
     - Description
   * - ``SIZE``
     - A total number of ranks on the local host.

**Description**

Set this environment variable to specify a total number of ranks on a local host.

CCL_LOCAL_RANK
**************

**Syntax**
::

  CCL_LOCAL_RANK=<value>

**Arguments**

.. list-table::
   :widths: 25 50
   :header-rows: 1
   :align: left

   * - <value>
     - Description
   * - ``RANK``
     - Rank number of the current process on the local host.


**Description**

Set this environment variable to specify the rank number of the current process in the local host.

Multi-NIC
#########


``CCL_MNIC``, ``CCL_MNIC_NAME`` and ``CCL_MNIC_COUNT`` define filters to select multiple NICs.
|product_short| workers will be pinned on selected NICs in a round-robin way.


CCL_MNIC
********
**Syntax**

::

  CCL_MNIC=<value>

**Arguments**

.. list-table::
   :widths: 25 50
   :header-rows: 1
   :align: left

   * - <value>
     - Description
   * - ``global``
     - Select all NICs available on the node.
   * - ``local``
     - Select all NICs local for the NUMA node that corresponds to process pinning.
   * - ``none``
     - Disable special NIC selection, use a single default NIC (**default**).

**Description**

Set this environment variable to control multi-NIC selection by NIC locality.


CCL_MNIC_NAME
*************
**Syntax**

::

  CCL_MNIC_NAME=<namelist>

**Arguments**

.. list-table::
   :widths: 25 50
   :header-rows: 1
   :align: left

   * - <namelist>
     - Description
   * - ``<namelist>``
     - A comma-separated list of NIC full names or prefixes to filter NICs.
       Use the ``^`` symbol to exclude NICs starting with the specified prefixes. For example,
       if you provide a list ``mlx5_0,mlx5_1,^mlx5_2``, NICs with the names ``mlx5_0`` and ``mlx5_1``
       will be selected, while ``mlx5_2`` will be excluded from the selection.

**Description**

Set this environment variable to control multi-NIC selection by NIC names.


CCL_MNIC_COUNT
**************

**Syntax**

::

  CCL_MNIC_COUNT=<value>

**Arguments**

.. list-table::
   :widths: 25 50
   :header-rows: 1
   :align: left

   * - <value>
     - Description
   * - ``N``
     - The maximum number of NICs that should be selected for |product_short| workers.
       If not specified then equal to the number of |product_short| workers.

**Description**

Set this environment variable to specify the maximum number of NICs to be
selected. The actual number of NICs selected may be smaller due to limitations
on transport level or system configuration.

Inter Process Communication (IPC)
#################################

CCL_ZE_CACHE_OPEN_IPC_HANDLES_THRESHOLD
***************************************

**Syntax**

::

  CCL_ZE_CACHE_OPEN_IPC_HANDLES_THRESHOLD=<value>

.. list-table::
   :widths: 25 50
   :header-rows: 1
   :align: left

   * - <value>
     - Description
   * - ``N``
     - The number IPC handles in the receiver cache. The default value is 1000.

**Description**

Use this environment variable to change the number of IPC
handles opened with ``zeMemOpenIpcHandle()`` that oneCCL maintains in its receiving
cache. IPC handles refer to `Level Zero Memory IPCs
<https://spec.oneapi.io/level-zero/latest/core/PROG.html#memory-1>`_.

The IPC handles opened with ``zeMemOpenIpcHandle()`` are stored by oneCCL in
the receiving cache. However, when the number of opened IPC handles exceeds the
specified threshold, the cache will evict a handle using a LRU (Last Recently
Used) policy. Starting with version 2021.10, the default value is 1000.


CCL_ZE_CACHE_GET_IPC_HANDLES_THRESHOLD
**************************************

**Syntax**

::

  CCL_ZE_CACHE_GET_IPC_HANDLES_THRESHOLD=<value>

.. list-table::
   :widths: 25 50
   :header-rows: 1
   :align: left

   * - <value>
     - Description
   * - ``N``
     -	The number IPC handles in the receiver cache. The default value is 1000.

**Description**

Use this environment variable to change the number of IPC handles obtained with
``zeMemGetIpcHandle()`` that oneCCL maintains in its sender cache. IPC handles
refer to `Level Zero Memory IPCs <https://spec.oneapi.io/level-zero/latest/core/PROG.html#memory-1>`_.

The IPC handles obtained with ``zeMemGetIpcHandle()`` are stored by oneCCL in the
sender cache. However, when the number of get IPC handles exceeds the specified
threshold, the cache will evict a handle using a LRU (Last Recently Used)
policy. The default value is 1000.


.. _low-precision-datatypes:

Low-precision datatypes
#######################


The group of environment variables to control processing of low-precision datatypes.


CCL_BF16
********
**Syntax**

::

  CCL_BF16=<value>

**Arguments**

.. list-table::
   :widths: 25 50
   :header-rows: 1
   :align: left

   * - <value>
     - Description
   * - ``avx512f``
     - Select implementation based on ``AVX512F`` instructions.
   * - ``avx512bf``
     - Select implementation based on ``AVX512_BF16`` instructions.

**Description**

Set this environment variable to select implementation for BF16 <-> FP32
conversion on reduction phase of collective operation. The default value
depends on instruction set support on specific CPU. ``AVX512_BF16``-based
implementation has precedence over ``AVX512F``-based one.


CCL_FP16
********
**Syntax**

::

  CCL_FP16=<value>

**Arguments**

.. list-table::
   :widths: 25 50
   :header-rows: 1
   :align: left

   * - <value>
     - Description
   * - ``f16c``
     - Select implementation based on ``F16C`` instructions.
   * - ``avx512f``
     - Select implementation based on ``AVX512F`` instructions.
   * - ``avx512fp16``
     - Select implementation based on ``AVX512FP16`` instructions.

**Description**

Set this environment variable to select implementation for on reduction phase of collective operation.
``AVX512FP16`` uses native FP16 numeric operations for reduction.
``AVX512F`` and ``F16C`` use FP16 <-> FP32 conversion operations to perform the reduction.
The default value depends on instruction set support on specific CPU.
``AVX512FP16``-based implementation has precedence over ``AVX512F`` and ``F16C``-based one.


CCL_LOG_LEVEL
#############

**Syntax**

::

  CCL_LOG_LEVEL=<value>

**Arguments**

.. list-table::
   :header-rows: 1
   :align: left

   * - <value>
   * - ``error``
   * - ``warn`` (**default**)
   * - ``info``
   * - ``debug``
   * - ``trace``

**Description**

Set this environment variable to control logging level.


CCL_ITT_LEVEL
#############

**Syntax**

::

  CCL_ITT_LEVEL=<value>

**Arguments**

.. list-table::
   :widths: 25 50
   :header-rows: 1
   :align: left

   * - <value>
     - Description
   * - ``1``
     - Enable support for ITT profiling.
   * - ``0``
     - Disable support for ITT profiling (**default**).

**Description**

Set this environment variable to specify Intel\ |reg|\  Instrumentation and Tracing Technology (ITT) profiling level.
Once the environment variable is enabled (value > 0), it is possible to collect and display profiling
data for |product_short| using tools such as Intel\ |reg|\  VTune\ |tm|\  Profiler.


Fusion
######


The group of environment variables to control fusion of collective operations.


CCL_FUSION
**********

**Syntax**

::

  CCL_FUSION=<value>

**Arguments**

.. list-table::
   :widths: 25 50
   :header-rows: 1
   :align: left

   * - <value>
     - Description
   * - ``1``
     - Enable fusion of collective operations
   * - ``0``
     - Disable fusion of collective operations (**default**)

**Description**

Set this environment variable to control fusion of collective operations.
The real fusion depends on additional settings described below.

.. _CCL_FUSION_BYTES_THRESHOLD:

CCL_FUSION_BYTES_THRESHOLD
**************************

**Syntax**

::

  CCL_FUSION_BYTES_THRESHOLD=<value>

**Arguments**

.. list-table::
   :widths: 25 50
   :header-rows: 1
   :align: left

   * - <value>
     - Description
   * - ``SIZE``
     - Bytes threshold for a collective operation. If the size of a communication buffer in bytes is less than or equal
       to ``SIZE``, then |product_short| fuses this operation with the other ones.

**Description**

Set this environment variable to specify the threshold of the number of bytes for a collective operation to be fused.

.. _CCL_FUSION_COUNT_THRESHOLD:

CCL_FUSION_COUNT_THRESHOLD
**************************

**Syntax**

::

  CCL_FUSION_COUNT_THRESHOLD=<value>

**Arguments**

.. list-table::
   :widths: 25 50
   :header-rows: 1
   :align: left

   * - <value>
     - Description
   * - ``COUNT``
     - The threshold for the number of collective operations.
       |product_short| can fuse together no more than ``COUNT`` operations at a time.

**Description**

Set this environment variable to specify count threshold for a collective operation to be fused.


.. _CCL_FUSION_CYCLE_MS:

CCL_FUSION_CYCLE_MS
*******************

**Syntax**

::

  CCL_FUSION_CYCLE_MS=<value>

**Arguments**

.. list-table::
   :widths: 25 50
   :header-rows: 1
   :align: left

   * - <value>
     - Description
   * - ``MS``
     - The frequency of checking for collectives operations to be fused, in milliseconds:

       - Small ``MS`` value can improve latency.
       - Large ``MS`` value can help to fuse larger number of operations at a time.

**Description**

Set this environment variable to specify the frequency of checking for collectives operations to be fused.

.. _CCL_PRIORITY:

CCL_PRIORITY
############

**Syntax**

::

  CCL_PRIORITY=<value>

**Arguments**

.. list-table::
   :widths: 25 50
   :header-rows: 1
   :align: left

   * - <value>
     - Description
   * - ``direct``
     - You have to explicitly specify priority using ``priority``.
   * - ``lifo``
     - Priority is implicitly increased on each collective call. You do not have to specify priority.
   * - ``none``
     - Disable prioritization (**default**).

**Description**

Set this environment variable to control priority mode of collective operations.


CCL_MAX_SHORT_SIZE
##################

**Syntax**

::

  CCL_MAX_SHORT_SIZE=<value>

**Arguments**

.. list-table::
   :widths: 25 50
   :header-rows: 1
   :align: left

   * - <value>
     - Description
   * - ``SIZE``
     - Bytes threshold for a collective operation (``0`` if not specified).
       If the size of a communication buffer in bytes is less than or equal to
       ``SIZE``, then |product_short| does not split operation between workers.
       Applicable for ``ALLREDUCE``, ``REDUCE`` and ``BROADCAST``.

**Description**

Set this environment variable to specify the threshold of the number of bytes for a collective operation to be split.


CCL_SYCL_OUTPUT_EVENT
#####################

**Syntax**

::

  CCL_SYCL_OUTPUT_EVENT=<value>

**Arguments**

.. list-table::
   :widths: 25 50
   :header-rows: 1
   :align: left

   * - <value>
     - Description
   * - ``1``
     - Enable support for SYCL output event (**default**).
   * - ``0``
     - Disable support for SYCL output event.

**Description**

Set this environment variable to control support for SYCL output event.
Once the support is enabled, you can retrieve SYCL output event from |product_short| event using ``get_native()`` method.
|product_short| event must be associated with |product_short| communication operation.


CCL_ZE_LIBRARY_PATH
###################

**Syntax**

::

  CCL_ZE_LIBRARY_PATH=<value>

**Arguments**

.. list-table::
   :widths: 25 50
   :header-rows: 1
   :align: left

   * - <value>
     - Description
   * - ``PATH/NAME``
     - Specify the name and full path to the ``Level-Zero`` library for dynamic loading by |product_short|.

**Description**

Set this environment variable to specify the name and full path to
``Level-Zero`` library. The path should be absolute and validated. Set this
variable if ``Level-Zero`` is not located in the default path. By default
|product_short| uses ``libze_loader.so`` name for dynamic loading.


Point-To-Point Operations
#########################

CCL_RECV
********

**Syntax**

::

  CCL_RECV=<value>

**Arguments**

.. list-table::
   :widths: 25 50
   :header-rows: 1
   :align: left

   * - <value>
     - Description
   * - ``direct``
     - Based on the MPI*/OFI* transport layer.
   * - ``topo``
     - Uses Intel(R) Xe Link technology across GPUs in a multi-GPU node. The default for GPU buffers.
   * - ``offload``
     - Based on the MPI*/OFI* transport layer and GPU RDMA when supported by the hardware.



CCL_SEND
********

**Syntax**

::

  CCL_SEND=<value>

**Arguments**

.. list-table::
   :widths: 25 50
   :header-rows: 1
   :align: left

   * - <value>
     - Description
   * - ``direct``
     - Based on the MPI*/OFI* transport layer.
   * - ``topo``
     - Uses Intel(R) Xe Link technology across GPUs in a multi-GPU node. The default for GPU buffers.
   * - ``offload``
     - Based on the MPI*/OFI* transport layer and GPU RDMA when supported by the hardware.



CCL_ZE_TMP_BUF_SIZE
###################

**Syntax**

::

  CCL_ZE_TMP_BUF_SIZE=<value>

**Arguments**

.. list-table::
   :widths: 25 50
   :header-rows: 1
   :align: left

   * - <value>
     - Description
   * - ``N``
     - Size of the temporary buffer (in bytes) oneCCL uses to perform
       collective operations with topo algorithm and Level Zero path. Default is 536870912, that is, 512 MBs.


**Description**

Set this environment variable to change the size of the temporary buffer used
by the topo algorithm in the Level Zero path. The value is specified in bytes.
The default value is 536870912.

You can tune the value of this variable depending on the system memory
available, the memory the application requires, and the message size of the
collectives used. With larger values, oneCCL consumes more memory but can
provide higher performance. Similarly, small values will reduce memory
utilization, but can degrade performance.


