#include <torch/csrc/Exceptions.h>
#include <torch/csrc/python_headers.h>

#include <cstdarg>
#include <exception>
#include <sstream>
#include <utility>
#include <vector>

#include <fmt/format.h>
#include <torch/csrc/THP.h>

#include <c10/util/StringUtil.h>

PyObject *THPException_FatalError, *THPException_LinAlgError,
    *THPException_OutOfMemoryError;

#define ASSERT_TRUE(cond) \
  if (!(cond))            \
  return false
bool THPException_init(PyObject* module) {
  ASSERT_TRUE(
      THPException_FatalError =
          PyErr_NewException("torch.FatalError", nullptr, nullptr));
  ASSERT_TRUE(
      PyModule_AddObject(module, "FatalError", THPException_FatalError) == 0);

  // Set the doc string here since _add_docstr throws malloc errors if tp_doc is
  // modified for an error class.
  ASSERT_TRUE(
      THPException_LinAlgError = PyErr_NewExceptionWithDoc(
          "torch._C._LinAlgError",
          "Error raised by torch.linalg function when the cause of error is a numerical inconsistency in the data.\n \
For example, you can the torch.linalg.inv function will raise torch.linalg.LinAlgError when it finds that \
a matrix is not invertible.\n \
\n\
Example:\n \
>>> # xdoctest: +REQUIRES(env:TORCH_DOCKTEST_LAPACK)\n \
>>> matrix = torch.eye(3, 3)\n \
>>> matrix[-1, -1] = 0\n \
>>> matrix\n \
    tensor([[1., 0., 0.],\n \
            [0., 1., 0.],\n \
            [0., 0., 0.]])\n \
>>> torch.linalg.inv(matrix)\n \
Traceback (most recent call last):\n \
File \"<stdin>\", line 1, in <module>\n \
torch._C._LinAlgError: torch.linalg.inv: The diagonal element 3 is zero, the inversion\n \
could not be completed because the input matrix is singular.",
          PyExc_RuntimeError,
          nullptr));
  ASSERT_TRUE(
      PyModule_AddObject(module, "_LinAlgError", THPException_LinAlgError) ==
      0);

  ASSERT_TRUE(
      THPException_OutOfMemoryError = PyErr_NewExceptionWithDoc(
          "torch.cuda.OutOfMemoryError",
          "Exception raised when CUDA is out of memory",
          PyExc_RuntimeError,
          nullptr));
  ASSERT_TRUE(
      PyModule_AddObject(
          module, "_OutOfMemoryError", THPException_OutOfMemoryError) == 0);

  return true;
}

namespace torch {

void processErrorMsgInplace(std::string& str) {
  // Translate Aten types to their respective pytorch ones
  constexpr std::array<std::pair<c10::string_view, c10::string_view>, 64>
      changes{{
          {"Variable[SparseCUDAByteType]", "torch.cuda.sparse.ByteTensor"},
          {"Variable[SparseCUDACharType]", "torch.cuda.sparse.CharTensor"},
          {"Variable[SparseCUDADoubleType]", "torch.cuda.sparse.DoubleTensor"},
          {"Variable[SparseCUDAFloatType]", "torch.cuda.sparse.FloatTensor"},
          {"Variable[SparseCUDAIntType]", "torch.cuda.sparse.IntTensor"},
          {"Variable[SparseCUDALongType]", "torch.cuda.sparse.LongTensor"},
          {"Variable[SparseCUDAShortType]", "torch.cuda.sparse.ShortTensor"},
          {"Variable[SparseCUDAHalfType]", "torch.cuda.sparse.HalfTensor"},
          {"Variable[SparseCPUByteType]", "torch.sparse.ByteTensor"},
          {"Variable[SparseCPUCharType]", "torch.sparse.CharTensor"},
          {"Variable[SparseCPUDoubleType]", "torch.sparse.DoubleTensor"},
          {"Variable[SparseCPUFloatType]", "torch.sparse.FloatTensor"},
          {"Variable[SparseCPUIntType]", "torch.sparse.IntTensor"},
          {"Variable[SparseCPULongType]", "torch.sparse.LongTensor"},
          {"Variable[SparseCPUShortType]", "torch.sparse.ShortTensor"},
          {"Variable[SparseCPUHalfType]", "torch.sparse.HalfTensor"},
          {"Variable[CUDAByteType]", "torch.cuda.ByteTensor"},
          {"Variable[CUDACharType]", "torch.cuda.CharTensor"},
          {"Variable[CUDADoubleType]", "torch.cuda.DoubleTensor"},
          {"Variable[CUDAFloatType]", "torch.cuda.FloatTensor"},
          {"Variable[CUDAIntType]", "torch.cuda.IntTensor"},
          {"Variable[CUDALongType]", "torch.cuda.LongTensor"},
          {"Variable[CUDAShortType]", "torch.cuda.ShortTensor"},
          {"Variable[CUDAHalfType]", "torch.cuda.HalfTensor"},
          {"Variable[CPUByteType]", "torch.ByteTensor"},
          {"Variable[CPUCharType]", "torch.CharTensor"},
          {"Variable[CPUDoubleType]", "torch.DoubleTensor"},
          {"Variable[CPUFloatType]", "torch.FloatTensor"},
          {"Variable[CPUIntType]", "torch.IntTensor"},
          {"Variable[CPULongType]", "torch.LongTensor"},
          {"Variable[CPUShortType]", "torch.ShortTensor"},
          {"Variable[CPUHalfType]", "torch.HalfTensor"},
          {"SparseCUDAByteType", "torch.cuda.sparse.ByteTensor"},
          {"SparseCUDACharType", "torch.cuda.sparse.CharTensor"},
          {"SparseCUDADoubleType", "torch.cuda.sparse.DoubleTensor"},
          {"SparseCUDAFloatType", "torch.cuda.sparse.FloatTensor"},
          {"SparseCUDAIntType", "torch.cuda.sparse.IntTensor"},
          {"SparseCUDALongType", "torch.cuda.sparse.LongTensor"},
          {"SparseCUDAShortType", "torch.cuda.sparse.ShortTensor"},
          {"SparseCUDAHalfType", "torch.cuda.sparse.HalfTensor"},
          {"SparseCPUByteType", "torch.sparse.ByteTensor"},
          {"SparseCPUCharType", "torch.sparse.CharTensor"},
          {"SparseCPUDoubleType", "torch.sparse.DoubleTensor"},
          {"SparseCPUFloatType", "torch.sparse.FloatTensor"},
          {"SparseCPUIntType", "torch.sparse.IntTensor"},
          {"SparseCPULongType", "torch.sparse.LongTensor"},
          {"SparseCPUShortType", "torch.sparse.ShortTensor"},
          {"SparseCPUHalfType", "torch.sparse.HalfTensor"},
          {"CUDAByteType", "torch.cuda.ByteTensor"},
          {"CUDACharType", "torch.cuda.CharTensor"},
          {"CUDADoubleType", "torch.cuda.DoubleTensor"},
          {"CUDAFloatType", "torch.cuda.FloatTensor"},
          {"CUDAIntType", "torch.cuda.IntTensor"},
          {"CUDALongType", "torch.cuda.LongTensor"},
          {"CUDAShortType", "torch.cuda.ShortTensor"},
          {"CUDAHalfType", "torch.cuda.HalfTensor"},
          {"CPUByteType", "torch.ByteTensor"},
          {"CPUCharType", "torch.CharTensor"},
          {"CPUDoubleType", "torch.DoubleTensor"},
          {"CPUFloatType", "torch.FloatTensor"},
          {"CPUIntType", "torch.IntTensor"},
          {"CPULongType", "torch.LongTensor"},
          {"CPUShortType", "torch.ShortTensor"},
          {"CPUHalfType", "torch.HalfTensor"},
      }};

  // Avoid doing any work if no types need translated
  if (str.find("Type") == str.npos) {
    return;
  }
  for (const auto& it : changes) {
    c10::ReplaceAll(str, it.first, it.second);
  }
}

std::string processErrorMsg(std::string str) {
  processErrorMsgInplace(str);
  return str;
}

static std::string formatMessage(const char* format, va_list fmt_args) {
  static const size_t ERROR_BUF_SIZE = 1024;
  // NOLINTNEXTLINE(modernize-avoid-c-arrays,cppcoreguidelines-avoid-c-arrays)
  char error_buf[ERROR_BUF_SIZE];
  vsnprintf(error_buf, ERROR_BUF_SIZE, format, fmt_args);

  // Ensure that the string is null terminated
  error_buf[sizeof(error_buf) / sizeof(*error_buf) - 1] = 0;

  return std::string(error_buf);
}

void translate_exception_to_python(const std::exception_ptr& e_ptr) {
  try {
    TORCH_INTERNAL_ASSERT(
        e_ptr,
        "translate_exception_to_python "
        "called with invalid exception pointer");
    std::rethrow_exception(e_ptr);
  }
  CATCH_ALL_ERRORS(return )
}

IndexError::IndexError(const char* format, ...) {
  va_list fmt_args;
  va_start(fmt_args, format);
  msg = formatMessage(format, fmt_args);
  va_end(fmt_args);
}

TypeError::TypeError(const char* format, ...) {
  va_list fmt_args;
  va_start(fmt_args, format);
  msg = formatMessage(format, fmt_args);
  va_end(fmt_args);
}

ValueError::ValueError(const char* format, ...) {
  va_list fmt_args;
  va_start(fmt_args, format);
  msg = formatMessage(format, fmt_args);
  va_end(fmt_args);
}

AttributeError::AttributeError(const char* format, ...) {
  va_list fmt_args;
  va_start(fmt_args, format);
  msg = formatMessage(format, fmt_args);
  va_end(fmt_args);
}

LinAlgError::LinAlgError(const char* format, ...) {
  va_list fmt_args;
  va_start(fmt_args, format);
  msg = formatMessage(format, fmt_args);
  va_end(fmt_args);
}

void PyWarningHandler::InternalHandler::process(
    const c10::SourceLocation& source_location,
    const std::string& msg,
    const bool verbatim) {
  warning_buffer_.push_back({source_location, msg, verbatim});
};

PyWarningHandler::PyWarningHandler() noexcept(true)
    : prev_handler_(c10::Warning::get_warning_handler()), in_exception_(false) {
  c10::Warning::set_warning_handler(&internal_handler_);
}

/// See NOTE [ Conversion Cpp Python Warning ] for noexcept justification
/// NOLINTNEXTLINE(bugprone-exception-escape)
PyWarningHandler::~PyWarningHandler() noexcept(false) {
  c10::Warning::set_warning_handler(prev_handler_);
  auto& warning_buffer = internal_handler_.warning_buffer_;

  if (warning_buffer.size() > 0) {
    // NOLINTNEXTLINE(cppcoreguidelines-init-variables)
    PyObject *type, *value, *traceback;
    pybind11::gil_scoped_acquire gil;
    auto result = 0;
    if (in_exception_) {
      // This (combined with PyErr_Restore below) also works when no python
      // error has been set yet
      PyErr_Fetch(&type, &value, &traceback);
    }
    for (auto& warning : warning_buffer) {
      auto source_location = warning.source_location_;
      auto& msg = warning.msg_;
      processErrorMsgInplace(msg);
      if (source_location.file == nullptr) {
        result = PyErr_WarnEx(PyExc_RuntimeWarning, msg.c_str(), 1);
      } else if (warning.verbatim_) {
        // Sets the source location from the warning
        // Note: PyErr_WarnExplicit will disregard Python's warning filter
        // and always appear. This is in contrast to PyErr_WarnEx,
        // which respects the warning filter.
        result = PyErr_WarnEx(PyExc_UserWarning, msg.c_str(), 1);
      } else {
        // Lets Python set the source location and puts the C++ warning
        // location into the message.
        fmt::memory_buffer buf;
        fmt::format_to(
            buf,
            FMT_STRING("{} (Triggered internally at {}:{}.)"),
            msg,
            source_location.file,
            source_location.line);
        buf.push_back('\0');
        result = PyErr_WarnEx(PyExc_UserWarning, buf.data(), 1);
      }
      if (result < 0) {
        if (in_exception_) {
          // PyErr_Print prints the traceback to sys.stderr and
          // clears the error indicator
          PyErr_Print();
        } else {
          break;
        }
      }
    }
    warning_buffer.clear();
    if ((result < 0) && (!in_exception_)) {
      /// A warning raised an error, we need to force the parent
      /// function to return an error code.
      throw python_error();
    }
    if (in_exception_) {
      // NOLINTNEXTLINE(clang-analyzer-core.CallAndMessage)
      PyErr_Restore(type, value, traceback);
    }
  }
}

} // namespace torch
