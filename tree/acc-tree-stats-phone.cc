// bin/acc-tree-stats.cc

// Copyright 2009-2011  Microsoft Corporation, GoVivace Inc.
//                2013  Johns Hopkins University (author: Daniel Povey)

// See ../../COPYING for clarification regarding multiple authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//  http://www.apache.org/licenses/LICENSE-2.0
//
// THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION ANY IMPLIED
// WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE,
// MERCHANTABLITY OR NON-INFRINGEMENT.
// See the Apache 2 License for the specific language governing permissions and
// limitations under the License.

#include "base/kaldi-common.h"
#include "util/common-utils.h"
#include "tree/context-dep.h"
#include "tree/build-tree-utils.h"
#include "hmm/transition-model.h"
#include "hmm/tree-accu.h"
#include "hmm/hmm-utils.h"

namespace kaldi {

static int32 MapPhone(const std::vector<int32> *phone_map,
                      int32 phone) {
  if (phone == 0 || phone_map == NULL) return phone;
  else if (phone < 0 || phone >= phone_map->size()) {
    KALDI_ERR << "Out-of-range phone " << phone << " bad --phone-map option?";
  }
  return (*phone_map)[phone];
}

void AccumulateTreeStatsPhone(const TransitionModel &trans_model,
                         BaseFloat var_floor,
                         int N,  // context window size.
                         int P,  // central position.
                         const std::vector<int32> &ci_phones,
                         const std::vector<int32> &alignment,
                         const Matrix<BaseFloat> &features,
                         const std::vector<int32> *phone_map,
                         std::map<EventType, GaussClusterable*> *stats) {

  KALDI_ASSERT(IsSortedAndUniq(ci_phones));
  std::vector<std::vector<int32> > split_alignment;
  bool ans = SplitToPhones(trans_model, alignment, &split_alignment);
  if (!ans) {
    KALDI_WARN << "AccumulateTreeStats: alignment appears to be bad, not using it";
    return;
  }
  int cur_pos = 0;
  int dim = features.NumCols();
  KALDI_ASSERT(features.NumRows() == static_cast<int32>(alignment.size()));
  for (int i = -N; i < static_cast<int>(split_alignment.size()); i++) {
    // consider window starting at i, only if i+P is within
    // list of phones.
    if (i + P >= 0 && i + P < static_cast<int>(split_alignment.size())) {
      int32 central_phone =
          MapPhone(phone_map,
                   trans_model.TransitionIdToPhone(split_alignment[i+P][0]));
      bool is_ctx_dep = ! std::binary_search(ci_phones.begin(),
                                             ci_phones.end(),
                                             central_phone);
      //make cd phone
      EventType evec;
      for (int j = 0; j < N; j++) {
        int phone;
        if (i + j >= 0 && i + j < static_cast<int>(split_alignment.size()))
          phone =
              MapPhone(phone_map,
                       trans_model.TransitionIdToPhone(split_alignment[i+j][0]));
        else
          phone = 0;  // ContextDependency class uses 0 to mean "out of window";
        // we also set the phone arbitrarily to 0
        if (is_ctx_dep || j == P)
          evec.push_back(std::make_pair(static_cast<EventKeyType>(j), static_cast<EventValueType>(phone)));
      }
      //binbin concatenating 3 states central phone as feature vector, 39 * 3
      int start = 0;
      std::vector <int> middle;
      for (int j = 1; j < static_cast<int>(split_alignment[i+P].size());j++) {
        int32 pdf_class_pre = trans_model.TransitionIdToPdfClass(split_alignment[i+P][j-1]);
        int32 pdf_class = trans_model.TransitionIdToPdfClass(split_alignment[i+P][j]);
        if (pdf_class != pdf_class_pre) {
            middle.push_back((start+j-1)/2);
            start = j;
        }
      }
      middle.push_back((start+split_alignment[i+P].size()-1)/2);
      KALDI_ASSERT(3 == middle.size()); //only 3 states
      std::sort(evec.begin(), evec.end());
      if (stats->count(evec) == 0) {
          (*stats)[evec] = new GaussClusterable(dim * 3, var_floor);
      }
      //concatenating feature
      //KALDI_LOG << features.NumRows() << " " \
                << cur_pos << " " << split_alignment[i+P].size() << " " \
                << middle[0] << " " <<  middle[1] << " " << middle[2];
      KALDI_ASSERT(middle[0] != middle[1]);
      KALDI_ASSERT(middle[1] != middle[2]);

      Vector<BaseFloat> concate_features(dim * 3);
      concate_features.Range(0, dim).CopyFromVec(features.Row(cur_pos+middle[0]));
      concate_features.Range(dim, dim).CopyFromVec(features.Row(cur_pos+middle[1]));
      concate_features.Range(2*dim, dim).CopyFromVec(features.Row(cur_pos+middle[2]));

      BaseFloat weight = 1.0;
      (*stats)[evec]->AddStats(concate_features, weight);
      cur_pos += split_alignment[i+P].size();

      //for (int j = 0; j < static_cast<int>(split_alignment[i+P].size());j++) {
      //  // for central phone of this window...
      //  EventType evec_more(evec);
      //  int32 pdf_class = trans_model.TransitionIdToPdfClass(split_alignment[i+P][j]);
      //  // pdf_class will normally by 0, 1 or 2 for 3-state HMM.
      //  std::pair<EventKeyType, EventValueType> pr(kPdfClass, pdf_class);
      //  evec_more.push_back(pr);
      //  std::sort(evec_more.begin(), evec_more.end());  // these must be sorted!
      //  if (stats->count(evec_more) == 0)
      //    (*stats)[evec_more] = new GaussClusterable(dim, var_floor);
      //  
      //  BaseFloat weight = 1.0;
      //  (*stats)[evec_more]->AddStats(features.Row(cur_pos), weight);
      //  cur_pos++;
      //}
    }
  }
  KALDI_ASSERT(cur_pos == static_cast<int>(alignment.size()));
}

}

/** @brief Accumulate tree statistics for decision tree training. The
program reads in a feature archive, and the corresponding alignments,
and generats the sufficient statistics for the decision tree
creation. Context width and central phone position are used to
identify the contexts.Transition model is used as an input to identify
the PDF's and the phones.  */
int main(int argc, char *argv[]) {
  using namespace kaldi;
  typedef kaldi::int32 int32;
  try {
    const char *usage =
        "Accumulate statistics for phonetic-context tree building.\n"
        "Usage:  acc-tree-stats [options] model-in features-rspecifier alignments-rspecifier [tree-accs-out]\n"
        "e.g.: \n"
        " acc-tree-stats 1.mdl scp:train.scp ark:1.ali 1.tacc\n";
    ParseOptions po(usage);
    bool binary = true;
    float var_floor = 0.01;
    string ci_phones_str;
    std::string phone_map_rxfilename;
    int N = 3;
    int P = 1;
    po.Register("binary", &binary, "Write output in binary mode");
    po.Register("var-floor", &var_floor, "Variance floor for tree clustering.");
    po.Register("ci-phones", &ci_phones_str, "Colon-separated list of integer "
                "indices of context-independent phones (after mapping, if "
                "--phone-map option is used).");
    po.Register("context-width", &N, "Context window size.");
    po.Register("central-position", &P, "Central context-window position "
                "(zero-based)");
    po.Register("phone-map", &phone_map_rxfilename,
                "File name containing old->new phone mapping (each line is: "
                "old-integer-id new-integer-id)");
    
    po.Read(argc, argv);

    if (po.NumArgs() < 3 || po.NumArgs() > 4) {
      po.PrintUsage();
      exit(1);
    }

    std::string model_filename = po.GetArg(1),
        feature_rspecifier = po.GetArg(2),
        alignment_rspecifier = po.GetArg(3),
        accs_out_wxfilename = po.GetOptArg(4);

    std::vector<int32> phone_map;
    if (phone_map_rxfilename != "") {  // read phone map.
      ReadPhoneMap(phone_map_rxfilename,
                   &phone_map);
    }
    
    std::vector<int32> ci_phones;
    if (ci_phones_str != "") {
      SplitStringToIntegers(ci_phones_str, ":", false, &ci_phones);
      std::sort(ci_phones.begin(), ci_phones.end());
      if (!IsSortedAndUniq(ci_phones) || ci_phones[0] == 0) {
        KALDI_ERR << "Invalid set of ci_phones: " << ci_phones_str;
      }
    }

    

    TransitionModel trans_model;
    {
      bool binary;
      Input ki(model_filename, &binary);
      trans_model.Read(ki.Stream(), binary);
      // There is more in this file but we don't need it.
    }

    SequentialBaseFloatMatrixReader feature_reader(feature_rspecifier);
    RandomAccessInt32VectorReader alignment_reader(alignment_rspecifier);

    std::map<EventType, GaussClusterable*> tree_stats;

    int num_done = 0, num_no_alignment = 0, num_other_error = 0;

    for (; !feature_reader.Done(); feature_reader.Next()) {
      std::string key = feature_reader.Key();
      if (!alignment_reader.HasKey(key)) {
        num_no_alignment++;
      } else {
        const Matrix<BaseFloat> &mat = feature_reader.Value();
        const std::vector<int32> &alignment = alignment_reader.Value(key);

        if (alignment.size() != mat.NumRows()) {
          KALDI_WARN << "Alignments has wrong size "<< (alignment.size())<<" vs. "<< (mat.NumRows());
          num_other_error++;
          continue;
        }

        ////// This is the important part of this program.  ////////
        AccumulateTreeStatsPhone(trans_model,
                            var_floor,
                            N,
                            P,
                            ci_phones,
                            alignment,
                            mat,
                            (phone_map_rxfilename != "" ? &phone_map : NULL),
                            &tree_stats);
        //AccumulateTreeStats(trans_model,
        //                    var_floor,
        //                    N,
        //                    P,
        //                    ci_phones,
        //                    alignment,
        //                    mat,
        //                    (phone_map_rxfilename != "" ? &phone_map : NULL),
        //                    &tree_stats);

        num_done++;
        if (num_done % 1000 == 0)
          KALDI_LOG << "Processed " << num_done << " utterances.";
      }
    }

    BuildTreeStatsType stats;  // vectorized form.

    for (std::map<EventType, GaussClusterable*>::const_iterator iter = tree_stats.begin();  
        iter != tree_stats.end();
        iter++ ) {
      stats.push_back(std::make_pair(iter->first, iter->second));
    }
    tree_stats.clear();

    {
      Output ko(accs_out_wxfilename, binary);
      WriteBuildTreeStats(ko.Stream(), binary, stats);
    }
    KALDI_LOG << "Accumulated stats for " << num_done << " files, "
              << num_no_alignment << " failed due to no alignment, "
              << num_other_error << " failed for other reasons.";
    KALDI_LOG << "Number of separate stats (context-dependent states) is "
              << stats.size();
    DeleteBuildTreeStats(&stats);
    if (num_done != 0) return 0;
    else return 1;
  } catch(const std::exception &e) {
    std::cerr << e.what();
    return -1;
  }
}


